/*
 * Copyright (C) 2010, 2012-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <drm/drmP.h>
#include <drm/mali_drm.h>
#include "mali_drv.h"

#define VIDEO_TYPE 0
#define MEM_TYPE 1

#define MALI_MM_ALIGN_SHIFT 4
#define MALI_MM_ALIGN_MASK ( (1 << MALI_MM_ALIGN_SHIFT) - 1)

struct mali_memblock {
	struct drm_mm_node mm_node;
	struct list_head owner_list;
};

static int mali_fb_init( struct drm_device *dev, void *data, struct drm_file *file_priv )
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	drm_mali_fb_t *fb = data;
	DRM_DEBUG("\n");

	mutex_lock(&dev->struct_mutex);
	drm_mm_init(&dev_priv->vram_mm, 0, fb->size >> MALI_MM_ALIGN_SHIFT);

	dev_priv->vram_initialized = 1;
	dev_priv->vram_offset = fb->offset;

	mutex_unlock(&dev->struct_mutex);
	DRM_DEBUG("offset = %lu, size = %lu\n", fb->offset, fb->size);

	return 0;
}

static int mali_drm_alloc(struct drm_device *dev, struct drm_file *file, void *data, int pool)
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	drm_mali_mem_t *mem = data;
	int retval = 0, user_key;
	struct mali_memblock *item;
	unsigned long offset;
	struct mali_file_private *file_priv = file->driver_priv;
	DRM_DEBUG("\n");

	if (pool > MEM_TYPE) {
		DRM_ERROR("Unknown memory type allocation\n");
		return -EINVAL;
	}
	mutex_lock(&dev->struct_mutex);

	if (0 == ((pool == VIDEO_TYPE) ? dev_priv->vram_initialized :
			dev_priv->mem_initialized)) {
		DRM_ERROR("Attempt to allocate from uninitialized memory manager.\n");
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}

	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (!item) {
		retval = -ENOMEM;
		goto fail_alloc;
	}

	mem->size = (mem->size + MALI_MM_ALIGN_MASK) >> MALI_MM_ALIGN_SHIFT;
	if (pool == MEM_TYPE) {
		retval = drm_mm_insert_node(&dev_priv->mem_mm,
					   &item->mm_node,
					   mem->size, 0,
					   DRM_MM_SEARCH_DEFAULT);
		offset = item->mm_node.start;
	} else {
		retval = drm_mm_insert_node(&dev_priv->vram_mm,
					   &item->mm_node,
					   mem->size, 0,
					   DRM_MM_SEARCH_DEFAULT);
		offset = item->mm_node.start;
	}
	if (retval)
		goto fail_alloc;

	retval = idr_alloc(&dev_priv->object_idr, item, 1, 0, GFP_KERNEL);
	if (retval < 0)
		goto fail_idr;
	user_key = retval;

	list_add(&item->owner_list, &file_priv->obj_list);
	mutex_unlock(&dev->struct_mutex);

	mem->offset = dev_priv->vram_offset + (offset << MALI_MM_ALIGN_SHIFT);
	mem->free = user_key;
	mem->size = mem->size << MALI_MM_ALIGN_SHIFT;

	return 0;

fail_idr:
	drm_mm_remove_node(&item->mm_node);
fail_alloc:
	kfree(item);
	mutex_unlock(&dev->struct_mutex);

	mem->offset = 0;
	mem->size = 0;
	mem->free = 0;

	DRM_DEBUG("alloc %d, size = %ld, offset = %ld\n", pool, mem->size, mem->offset);

	return retval;
}

static int mali_drm_free(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	drm_mali_mem_t *mem = data;
	struct mali_memblock *obj;
	DRM_DEBUG("\n");

	mutex_lock(&dev->struct_mutex);
	obj = idr_find(&dev_priv->object_idr, mem->free);
	if (obj == NULL) {
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}

	idr_remove(&dev_priv->object_idr, mem->free);
	list_del(&obj->owner_list);
	if (drm_mm_node_allocated(&obj->mm_node))
		drm_mm_remove_node(&obj->mm_node);
	kfree(obj);
	mutex_unlock(&dev->struct_mutex);
	DRM_DEBUG("free = 0x%lx\n", mem->free);

	return 0;
}

static int mali_fb_alloc(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	DRM_DEBUG("\n");
	return mali_drm_alloc(dev, file_priv, data, VIDEO_TYPE);
}

static int mali_ioctl_mem_init(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	drm_mali_mem_t *mem= data;
	dev_priv = dev->dev_private;
	DRM_DEBUG("\n");

	mutex_lock(&dev->struct_mutex);
	drm_mm_init(&dev_priv->mem_mm, 0, mem->size >> MALI_MM_ALIGN_SHIFT);

	dev_priv->mem_initialized = 1;
	dev_priv->mem_offset = mem->offset;
	mutex_unlock(&dev->struct_mutex);

	DRM_DEBUG("offset = %lu, size = %lu\n", mem->offset, mem->size);
	return 0;
}

static int mali_ioctl_mem_alloc(struct drm_device *dev, void *data,
			       struct drm_file *file_priv)
{

	DRM_DEBUG("\n");
	return mali_drm_alloc(dev, file_priv, data, MEM_TYPE);
}

int mali_idle(struct drm_device *dev)
{
	drm_mali_private_t *dev_priv = dev->dev_private;

	if (dev_priv->idle_fault)
		return 0;

	return 0;
}


void mali_lastclose(struct drm_device *dev)
{
	drm_mali_private_t *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");
	if (!dev_priv)
	       return;

	mutex_lock(&dev->struct_mutex);
	if (dev_priv->vram_initialized) {
		drm_mm_takedown(&dev_priv->vram_mm);
		dev_priv->vram_initialized = 0;
	}
	if (dev_priv->mem_initialized) {
		drm_mm_takedown(&dev_priv->mem_mm);
		dev_priv->mem_initialized = 0;
	}
	dev_priv->mmio = NULL;
	mutex_unlock(&dev->struct_mutex);
}

void mali_reclaim_buffers(struct drm_device * dev, struct drm_file *file)
{
	struct mali_file_private *file_priv = file->driver_priv;
	struct mali_memblock *entry, *next;
	DRM_DEBUG("\n");

	mutex_lock(&dev->struct_mutex);
	if (list_empty(&file_priv->obj_list)) {
		mutex_unlock(&dev->struct_mutex);
		return;
	}

	mali_idle(dev);

	list_for_each_entry_safe(entry, next, &file_priv->obj_list,
				 owner_list) {
		list_del(&entry->owner_list);
		if (drm_mm_node_allocated(&entry->mm_node))
			drm_mm_remove_node(&entry->mm_node);
		kfree(entry);
	}

	mutex_unlock(&dev->struct_mutex);

	return;
}

const struct drm_ioctl_desc mali_ioctls[] = {
	DRM_IOCTL_DEF(DRM_MALI_FB_ALLOC, mali_fb_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_MALI_FB_FREE, mali_drm_free, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_MALI_MEM_INIT, mali_ioctl_mem_init, DRM_AUTH | DRM_MASTER | DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_MALI_MEM_ALLOC, mali_ioctl_mem_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_MALI_MEM_FREE, mali_drm_free, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_MALI_FB_INIT, mali_fb_init, DRM_AUTH | DRM_MASTER | DRM_ROOT_ONLY),
};

int mali_max_ioctl = DRM_ARRAY_SIZE(mali_ioctls);

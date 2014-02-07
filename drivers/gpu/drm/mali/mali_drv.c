/*
 * Copyright (C) 2010, 2012-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/version.h>
#include <drm/drmP.h>
#include <drm/mali_drm.h>
#include "mali_drv.h"

static struct platform_device *mali_drm_pdev;
#if 0
static const struct drm_device_id dock_device_ids[] = {
	{"MALIDRM", 0},
	{"", 0},
};
#endif

static int mali_driver_load(struct drm_device *dev, unsigned long chipset)
{
	drm_mali_private_t *dev_priv;
	printk(KERN_ERR "DRM: mali_driver_load start\n");

	dev_priv = kmalloc(sizeof(drm_mali_private_t), GFP_KERNEL);
	if (dev_priv == NULL)
	       	return -ENOMEM;

	idr_init(&dev_priv->object_idr);
	dev->dev_private = (void *)dev_priv;

	printk(KERN_ERR "DRM: mali_driver_load done\n");

	return 0;
}

static int mali_driver_unload( struct drm_device *dev )
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	printk(KERN_ERR "DRM: mali_driver_unload start\n");

	idr_destroy(&dev_priv->object_idr);

	kfree(dev_priv);

	printk(KERN_ERR "DRM: mali_driver_unload done\n");

	return 0;
}

static const struct file_operations mali_driver_fops = {
        .owner = THIS_MODULE,
        .open = drm_open,
        .mmap = drm_mmap,
        .poll = drm_poll,
        .unlocked_ioctl = drm_ioctl,
        .release = drm_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
};

static int mali_driver_open(struct drm_device *dev, struct drm_file *file)
{
	struct mali_file_private *file_priv;

	DRM_DEBUG_DRIVER("\n");
	file_priv = kmalloc(sizeof(*file_priv), GFP_KERNEL);
	if (!file_priv)
		return -ENOMEM;

	file->driver_priv = file_priv;

	INIT_LIST_HEAD(&file_priv->obj_list);

	return 0;
}

void mali_driver_postclose(struct drm_device *dev, struct drm_file *file)
{
	struct via_file_private *file_priv = file->driver_priv;

	kfree(file_priv);
}



static struct drm_driver mali_drm_driver = 
{
	.driver_features = DRIVER_BUS_PLATFORM,
	.load = mali_driver_load,
	.unload = mali_driver_unload,
	.open = mali_driver_open,
	.preclose = mali_reclaim_buffers_locked,
	.postclose = mali_driver_postclose,
	.dma_quiescent = mali_idle,
	.lastclose = mali_lastclose,
	.ioctls = mali_ioctls,
	.fops = &mali_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static int mali_drm_platform_probe(struct platform_device *pdev)
{
        pr_info("DRM: %s: driver name: %s, version %d.%d\n", __func__, DRIVER_NAME, DRIVER_MAJOR, DRIVER_MINOR);
        return drm_platform_init(&mali_drm_driver, pdev);
}

static int mali_drm_platform_remove(struct platform_device *pdev)
{
        pr_info("DRM: mali_platform_drm_remove()\n");
        drm_platform_exit(&mali_drm_driver, pdev);
        return 0;
}

static struct platform_driver mali_drm_platform_driver = {
	.probe = mali_drm_platform_probe,
	.remove = mali_drm_platform_remove,
	.driver = {
		.name = "mali_drm",
		.owner = THIS_MODULE,
	},
};

static int __init mali_drm_init(void)
{
	struct platform_device *pdev;
	int ret;

        mali_drm_driver.num_ioctls = mali_max_ioctl;

        ret = platform_driver_register(&mali_drm_platform_driver);
	if (ret < 0)
		return ret;

        pdev = platform_device_register_simple("mali_drm", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		platform_driver_unregister(&mali_drm_platform_driver);
		return PTR_ERR(pdev);
	}

	mali_drm_pdev = pdev;

	return ret;
}

static void __exit mali_drm_exit(void)
{
        platform_device_unregister(mali_drm_pdev);
        platform_driver_unregister(&mali_drm_platform_driver);
}

module_init(mali_drm_init);
module_exit(mali_drm_exit);


MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_AUTHOR("ARM Ltd.");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");

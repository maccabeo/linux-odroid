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

static struct platform_device *pdev;
static int mali_platform_drm_probe(struct platform_device *dev)
{
        pr_info("DRM: mali_platform_drm_probe()\n");
        return mali_drm_init(dev);
}

static int mali_platform_drm_remove(struct platform_device *dev)
{
        pr_info("DRM: mali_platform_drm_remove()\n");
        mali_drm_exit(dev);
        return 0;
}

static int mali_platform_drm_suspend(struct platform_device *dev, pm_message_t state)
{
        pr_info("DRM: mali_platform_drm_suspend()\n");
        return 0;
}

static int mali_platform_drm_resume(struct platform_device *dev)
{
        pr_info("DRM: mali_platform_drm_resume()\n");
        return 0;
}


static char mali_drm_device_name[] = "mali_drm";
static struct platform_driver platform_drm_driver = {
	.probe = mali_platform_drm_probe,
	.remove = mali_platform_drm_remove,
	.suspend = mali_platform_drm_suspend,
	.resume = mali_platform_drm_resume,
	.driver = {
		.name = mali_drm_device_name,
		.owner = THIS_MODULE,
	},
};

#if 0
static const struct drm_device_id dock_device_ids[] = {
	{"MALIDRM", 0},
	{"", 0},
};
#endif

static int mali_driver_load(struct drm_device *dev, unsigned long chipset)
{
	#if 0
	unsigned long base, size;
	#endif
	drm_mali_private_t *dev_priv;
	printk(KERN_ERR "DRM: mali_driver_load start\n");

	dev_priv = kmalloc(sizeof(drm_mali_private_t), GFP_KERNEL);
	if ( dev_priv == NULL ) return -ENOMEM;

	dev->dev_private = (void *)dev_priv;

	if ( NULL == dev->platformdev )
	{
		dev->platformdev = platform_device_register_simple(mali_drm_device_name, 0, NULL, 0);
		pdev = dev->platformdev;
	}

	#if 0
	base = drm_get_resource_start(dev, 1 );
	size = drm_get_resource_len(dev, 1 );
	#endif
	idr_init(&dev_priv->object_idr);

	printk(KERN_ERR "DRM: mali_driver_load done\n");

	return 0;
}

static int mali_driver_unload( struct drm_device *dev )
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	printk(KERN_ERR "DRM: mali_driver_unload start\n");

	idr_destroy(&dev_priv->object_idr);

	kfree(dev_priv);
	//kfree( dev_priv );
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



static struct drm_driver driver = 
{
	.driver_features = DRIVER_BUS_PLATFORM,
	.load = mali_driver_load,
	.unload = mali_driver_unload,
	.context_dtor = NULL,
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

int mali_drm_init(struct platform_device *dev)
{
        pr_info("mali_drm_init(), driver name: %s, version %d.%d\n", DRIVER_NAME, DRIVER_MAJOR, DRIVER_MINOR);
        driver.num_ioctls = mali_max_ioctl;
        driver.kdriver.platform_device = dev;
        return drm_platform_init(&driver, dev);
}

void mali_drm_exit(struct platform_device *dev)
{
        drm_platform_exit(&driver, dev);
}

static int __init mali_platform_drm_init(void)
{
        pdev = platform_device_register_simple(mali_drm_device_name, 0, NULL, 0);
        return platform_driver_register(&platform_drm_driver);
}

static void __exit mali_platform_drm_exit(void)
{
        platform_device_unregister(pdev);
        platform_driver_unregister(&platform_drm_driver);
}

module_init(mali_platform_drm_init);
module_exit(mali_platform_drm_exit);


MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_AUTHOR("ARM Ltd.");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");

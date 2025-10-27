// SPDX-License-Identifier: GPL-2.0-only

#include <linux/version.h>
#include <linux/module.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
#include <drm/drm_fbdev_generic.h>
#else
#include <drm/drm_fbdev_ttm.h>
#endif
#include <drm/drm_file.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_simple_kms_helper.h>
#include <linux/mutex.h>

#include "ms912x.h"

/**
 * @brief Suspend function for ms912x USB device
 *
 * This function is called when the USB device is about to be suspended.
 * It suspends the DRM mode configuration to properly handle the power
 * management event.
 *
 * @param interface USB interface structure representing the device
 * @param message Power management message indicating the suspend state
 * @return 0 on success, negative error code on failure
 */
static int ms912x_usb_suspend(struct usb_interface *interface,
			      pm_message_t message)
{
	struct drm_device *dev = usb_get_intfdata(interface);
	
	// Добавляем дополнительную диагностику при приостановке работы
	if (dev) {
		struct ms912x_device *ms912x = to_ms912x(dev);
		if (ms912x) {
			pr_info("ms912x: [%s] suspending device operation\n", ms912x->device_name);
		}
	}
	
	return drm_mode_config_helper_suspend(dev);
}

static int ms912x_usb_resume(struct usb_interface *interface)
{
	struct drm_device *dev = usb_get_intfdata(interface);
	
	// Добавляем дополнительную диагностику при возобновлении работы
	if (dev) {
		struct ms912x_device *ms912x = to_ms912x(dev);
		if (ms912x) {
			pr_info("ms912x: [%s] resuming device operation\n", ms912x->device_name);
		}
	}
	
	return drm_mode_config_helper_resume(dev);
}

static struct drm_gem_object *
ms912x_driver_gem_prime_import(struct drm_device *dev, struct dma_buf *dma_buf)
{
	struct ms912x_device *ms912x = to_ms912x(dev);
	
	// Добавляем более подробную проверку и логирование
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer\n");
		return ERR_PTR(-EINVAL);
	}
	
	if (!ms912x->dmadev) {
		pr_warn("ms912x: buffer sharing not supported, dmadev is NULL\n");
		return ERR_PTR(-ENODEV);
	}

	if (!dma_buf) {
		pr_err("ms912x: invalid dma_buf pointer\n");
		return ERR_PTR(-EINVAL);
	}

	pr_debug("ms912x: [%s] importing DMA buffer: dmadev=%p\n",
		        ms912x->device_name, ms912x->dmadev);
	
	return drm_gem_prime_import_dev(dev, dma_buf, ms912x->dmadev);
}

DEFINE_DRM_GEM_FOPS(ms912x_driver_fops);
static const struct drm_driver driver = {
	.driver_features =
		DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,
	.fops = &ms912x_driver_fops,
	DRM_GEM_SHMEM_DRIVER_OPS,
	.gem_prime_import = ms912x_driver_gem_prime_import,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static const struct drm_mode_config_funcs ms912x_mode_config_funcs = {
	.fb_create = drm_gem_fb_create_with_dirty,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

struct ms912x_mode ms912x_mode_list[] = {
	/* Found in captures of the Windows driver */
	MS912X_MODE( 800,  600, 60, 0x4200, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1024,  768, 60, 0x4700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1152,  864, 60, 0x4c00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  720, 60, 0x4f00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  800, 60, 0x5700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  960, 60, 0x5b00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280, 1024, 60, 0x6000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1366,  768, 60, 0x6600, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1400, 1050, 60, 0x6700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1440,  900, 60, 0x6b00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1600,  900, 60, 0x7000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1680, 1050, 60, 0x7800, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1920, 1080, 60, 0x8100, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1920, 1200, 60, 0x8500, MS912X_PIXFMT_UYVY),
	MS912X_MODE(2048, 1152, 60, 0x8900, MS912X_PIXFMT_UYVY),
	MS912X_MODE(2560, 1440, 60, 0x9000, MS912X_PIXFMT_UYVY),

	/* Dumped from the device */
	MS912X_MODE( 720,  480, 60, 0x0200, MS912X_PIXFMT_UYVY),
	MS912X_MODE( 720,  576, 60, 0x1100, MS912X_PIXFMT_UYVY),
	MS912X_MODE( 640,  480, 60, 0x4000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1024,  768, 60, 0x4900, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  600, 60, 0x4e00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  768, 60, 0x5400, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280, 1024, 60, 0x6100, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1360,  768, 60, 0x6400, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1600, 1200, 60, 0x7300, MS912X_PIXFMT_UYVY),
	
	/* Дополнительные режимы для лучшей совместимости */
	MS912X_MODE( 800,  480, 60, 0x3000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1024,  600, 60, 0x4500, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1152,  864, 75, 0x4d00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  768, 60, 0x5300, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  800, 75, 0x5800, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1400, 1050, 75, 0x6800, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1680, 1050, 75, 0x7900, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1920, 1080, 50, 0x8000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1920, 1080, 75, 0x8200, MS912X_PIXFMT_UYVY),
};

static const struct ms912x_mode *
ms912x_get_mode(const struct drm_display_mode *mode)
{
	int i;
	int width = mode->hdisplay;
	int height = mode->vdisplay;
	int hz = drm_mode_vrefresh(mode);

	for (i = 0; i < ARRAY_SIZE(ms912x_mode_list); i++) {
		if (ms912x_mode_list[i].width == width &&
		    ms912x_mode_list[i].height == height &&
		    ms912x_mode_list[i].hz == hz) {
			return &ms912x_mode_list[i];
		}
	}
	
	// Добавляем дополнительную диагностику при неудачном поиске режима
	pr_debug("ms912x: mode not found for %dx%d@%dHz\n", width, height, hz);
	
	return ERR_PTR(-EINVAL);
}

static void ms912x_pipe_enable(struct drm_simple_display_pipe *pipe,
			       struct drm_crtc_state *crtc_state,
			       struct drm_plane_state *plane_state)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	struct drm_display_mode *mode = &crtc_state->mode;

	pr_info("ms912x: [%s] enabling display pipe, mode: %dx%d@%dHz\n",
	        ms912x->device_name, mode->hdisplay, mode->vdisplay, drm_mode_vrefresh(mode));
	
	ms912x_power_on(ms912x);

	if (crtc_state->mode_changed) {
		const struct ms912x_mode *ms_mode = ms912x_get_mode(mode);
		if (IS_ERR(ms_mode)) {
			pr_err("ms912x: [%s] failed to get mode for %dx%d@%dHz: %ld\n",
			       ms912x->device_name, mode->hdisplay, mode->vdisplay,
			       drm_mode_vrefresh(mode), PTR_ERR(ms_mode));
		} else {
			pr_info("ms912x: [%s] setting resolution: %dx%d@%dHz, mode=0x%04x\n",
			        ms912x->device_name, ms_mode->width, ms_mode->height,
			        ms_mode->hz, ms_mode->mode);
			ms912x_set_resolution(ms912x, ms_mode);
		}
	}
}

static void ms912x_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	
	// Добавляем дополнительную диагностику при отключении пайплайна
	pr_info("ms912x: [%s] disabling display pipe\n", ms912x->device_name);
	
	ms912x_power_off(ms912x);
}

static enum drm_mode_status
ms912x_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
		       const struct drm_display_mode *mode)
{
	const struct ms912x_mode *ret = ms912x_get_mode(mode);
	
	// Добавляем дополнительную диагностику при проверке валидности режима
	if (IS_ERR(ret)) {
		pr_debug("ms912x: mode %dx%d@%dHz is not supported\n",
		         mode->hdisplay, mode->vdisplay, drm_mode_vrefresh(mode));
		return MODE_BAD;
	} else {
		pr_debug("ms912x: mode %dx%d@%dHz is supported\n",
		         mode->hdisplay, mode->vdisplay, drm_mode_vrefresh(mode));
		return MODE_OK;
	}
}

static int ms912x_pipe_check(struct drm_simple_display_pipe *pipe,
			     struct drm_plane_state *new_plane_state,
			     struct drm_crtc_state *new_crtc_state)
{
	return 0;
}

#define INVALID_COORD 0x7fffffff

static void ms912x_update_rect_init(struct drm_rect *rect)
{
	rect->x1 = INVALID_COORD;
	rect->y1 = INVALID_COORD;
	rect->x2 = 0;
	rect->y2 = 0;
}

static bool ms912x_rect_is_valid(const struct drm_rect *rect)
{
	bool valid = (rect->x1 <= rect->x2 && rect->y1 <= rect->y2);
	pr_debug("ms912x: rectangle validity check: x1=%d, y1=%d, x2=%d, y2=%d, valid=%s\n",
	         rect->x1, rect->y1, rect->x2, rect->y2, valid ? "true" : "false");
	return valid;
}


static void ms912x_merge_rects(struct drm_rect *dest, const struct drm_rect *r1,
			       const struct drm_rect *r2)
{
	if (!ms912x_rect_is_valid(r1)) {
		*dest = *r2;
		return;
	}
	if (!ms912x_rect_is_valid(r2)) {
		*dest = *r1;
		return;
	}
	dest->x1 = min(r1->x1, r2->x1);
	dest->y1 = min(r1->y1, r2->y1);
	dest->x2 = max(r1->x2, r2->x2);
	dest->y2 = max(r1->y2, r2->y2);
}

static void ms912x_pipe_update(struct drm_simple_display_pipe *pipe,
			       struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = pipe->plane.state;
	struct drm_shadow_plane_state *shadow_plane_state =
		to_drm_shadow_plane_state(state);
	
	if (!state->fb)
		return;
		
	struct ms912x_device *ms912x = to_ms912x(state->fb->dev);
	struct drm_rect current_rect, rect;

	if (!ms912x_rect_is_valid(&ms912x->update_rect))
		ms912x_update_rect_init(&ms912x->update_rect);

	if (drm_atomic_helper_damage_merged(old_state, state, &current_rect)) {
		ms912x_merge_rects(&rect, &current_rect, &ms912x->update_rect);

		int ret = ms912x_fb_send_rect(
			state->fb, &shadow_plane_state->data[0], &rect);
		if (ret == 0) {
			ms912x_update_rect_init(&ms912x->update_rect);
		} else {
			ms912x_merge_rects(&ms912x->update_rect,
					   &ms912x->update_rect, &rect);
		}
	}
}

static const struct drm_simple_display_pipe_funcs ms912x_pipe_funcs = {
	.enable = ms912x_pipe_enable,
	.disable = ms912x_pipe_disable,
	.check = ms912x_pipe_check,
	.mode_valid = ms912x_pipe_mode_valid,
	.update = ms912x_pipe_update,
	DRM_GEM_SIMPLE_DISPLAY_PIPE_SHADOW_PLANE_FUNCS,
};

static const uint32_t ms912x_pipe_formats[] = {
	DRM_FORMAT_XRGB8888,
};


static DEFINE_MUTEX(yuv_lut_mutex);
static bool yuv_lut_initialized = false;
static atomic_t device_counter = ATOMIC_INIT(0);
/**
 * @brief Probe function for ms912x USB device
 *
 * This function is called when a USB device matching the ms912x driver is
 * connected to the system. It initializes all necessary components for the
 * device to function as a DRM display device.
 *
 * Initialization steps include:
 * - Initializing YUV lookup table if not already done
 * - Allocating and initializing DRM device structure
 * - Setting up DMA device for buffer sharing
 * - Initializing mode configuration with supported resolutions
 * - Setting initial display resolution
 * - Initializing USB transfer requests
 * - Initializing display connector
 * - Setting up simple display pipe
 * - Registering DRM device
 * - Setting up framebuffer device
 *
 * @param interface USB interface structure representing the connected device
 * @param id USB device ID matching the device to this driver
 * @return 0 on success, negative error code on failure
 */
static int ms912x_usb_probe(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	// Добавляем проверки на NULL
	if (!interface) {
		pr_err("ms912x: invalid interface pointer\n");
		return -EINVAL;
	}
	
	if (!id) {
		pr_err("ms912x: invalid id pointer\n");
		return -EINVAL;
	}

	mutex_lock(&yuv_lut_mutex);
	if (!yuv_lut_initialized) {
		pr_info("ms912x: initializing YUV lookup table\n");
		ms912x_init_yuv_lut();
		yuv_lut_initialized = true;
	}
	mutex_unlock(&yuv_lut_mutex);

	int ret;
	struct ms912x_device *ms912x;
	struct drm_device *dev;
	struct usb_device *usb_dev = interface_to_usbdev(interface);

	pr_info("ms912x: probe started for device %04x:%04x at %d-%d\n",
		id->idVendor, id->idProduct,
		usb_dev->bus->busnum, usb_dev->devnum);
	
	pr_debug("ms912x: devm_drm_dev_alloc begin\n");
	ms912x = devm_drm_dev_alloc(&interface->dev, &driver,
				    struct ms912x_device, drm);

	pr_debug("ms912x: devm_drm_dev_alloc end\n");

	if (IS_ERR(ms912x)) {
		ret = PTR_ERR(ms912x);
		pr_err("ms912x: devm_drm_dev_alloc failed: %d\n", ret);
		return ret;
	}

	// Инициализируем уникальный идентификатор устройства
	ms912x->device_id = atomic_inc_return(&device_counter);
	snprintf(ms912x->device_name, sizeof(ms912x->device_name),
		 "ms912x-%u", ms912x->device_id);
	
	pr_info("ms912x: assigned device ID %u, name %s\n",
		ms912x->device_id, ms912x->device_name);

	ms912x->intf = interface;
	dev = &ms912x->drm;

	pr_debug("ms912x: usb_intf_get_dma_device\n");
	ms912x->dmadev = usb_intf_get_dma_device(interface);

	if (!ms912x->dmadev) {
		pr_warn("ms912x: buffer sharing not supported\n");
		drm_warn(dev, "buffer sharing not supported");
	}

	pr_debug("ms912x: drmm_mode_config_init begin\n");
	ret = drmm_mode_config_init(dev);
	if (ret) {
		pr_err("ms912x: drmm_mode_config_init failed: %d\n", ret);
		goto err_put_device;
	}

	pr_debug("ms912x: set dev->mode_config\n");

	dev->mode_config.min_width = 0;
	dev->mode_config.max_width = 2048;
	dev->mode_config.min_height = 0;
	dev->mode_config.max_height = 2048;
	dev->mode_config.funcs = &ms912x_mode_config_funcs;
	
	pr_info("ms912x: [%s] mode_config initialized: min_width=%d, max_width=%d, min_height=%d, max_height=%d\n",
	        ms912x->device_name,
	        dev->mode_config.min_width,
	        dev->mode_config.max_width,
	        dev->mode_config.min_height,
	        dev->mode_config.max_height);

	pr_debug("ms912x: set_resolution begin\n");
	ret = ms912x_set_resolution(ms912x, &ms912x_mode_list[0]);
	if (ret) {
		pr_err("ms912x: set_resolution failed: %d\n", ret);
		goto err_mode_config_cleanup;
	}
	pr_debug("ms912x: set_resolution end\n");

	pr_debug("ms912x: init_request [0] \n");
	ret = ms912x_init_request(ms912x, &ms912x->requests[0],
				  2048 * 2048 * 2);
	if (ret) {
		pr_err("ms912x: init_request [0] failed: %d\n", ret);
		goto err_mode_config_cleanup;
	}

	pr_debug("ms912x: init_request [1] \n");
	ret = ms912x_init_request(ms912x, &ms912x->requests[1],
				  2048 * 2048 * 2);
	if (ret) {
		pr_err("ms912x: init_request [1] failed: %d\n", ret);
		goto err_free_request_0;
	}

	pr_debug("ms912x: complete request [1] \n");
	complete(&ms912x->requests[1].done);

	pr_debug("ms912x: connector_init \n");
	ret = ms912x_connector_init(ms912x);
	if (ret) {
		pr_err("ms912x: connector_init failed: %d\n", ret);
		goto err_free_request_1;
	}

	pr_debug("ms912x: drm_simple_display_pipe_init \n");
	ret = drm_simple_display_pipe_init(&ms912x->drm, &ms912x->display_pipe,
					   &ms912x_pipe_funcs,
					   ms912x_pipe_formats,
					   ARRAY_SIZE(ms912x_pipe_formats),
					   NULL, &ms912x->connector);
	if (ret) {
		pr_err("ms912x: [%s] failed to initialize display pipe: %d\n",
		       ms912x->device_name, ret);
		goto err_free_request_1;
	}
	
	pr_info("ms912x: [%s] display pipe initialized successfully\n", ms912x->device_name);

	pr_debug("ms912x: drm_plane_enable_fb_damage_clips \n");
	drm_plane_enable_fb_damage_clips(&ms912x->display_pipe.plane);

	pr_debug("ms912x: drm_mode_config_reset \n");
	drm_mode_config_reset(dev);

	pr_debug("ms912x: usb_set_intfdata \n");
	usb_set_intfdata(interface, ms912x);
	pr_debug("ms912x: drm_kms_helper_poll_init \n");
	drm_kms_helper_poll_init(dev);

	dev->dev_private = ms912x;

	pr_debug("ms912x: drm_dev_register \n");
	ret = drm_dev_register(dev, 0);
	if (ret) {
		pr_err("ms912x: [%s] drm_dev_register failed: %d\n",
		       ms912x->device_name, ret);
		goto err_kms_poll_fini;
	}
	
	pr_info("ms912x: [%s] drm device registered successfully\n", ms912x->device_name);

	pr_info("ms912x: drm_fbdev_generic_setup \n");
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
	drm_fbdev_generic_setup(dev, 0);
#else
	drm_fbdev_ttm_setup(dev, 0);
#endif
	
	pr_info("ms912x: [%s] framebuffer device setup completed\n", ms912x->device_name);

	pr_info("ms912x: probe completed successfully for device %s\n", ms912x->device_name);
	
	// Запускаем диагностику устройства после успешной инициализации
	int diag_result = ms912x_run_diagnostics(ms912x);
	if (diag_result < 0) {
		pr_warn("ms912x: [%s] diagnostics failed: %d\n",
		        ms912x->device_name, diag_result);
	} else {
		pr_info("ms912x: [%s] diagnostics passed\n", ms912x->device_name);
	}
	
	return 0;

err_kms_poll_fini:
	drm_kms_helper_poll_fini(dev);
err_free_request_1:
	ms912x_free_request(&ms912x->requests[1]);
err_free_request_0:
	ms912x_free_request(&ms912x->requests[0]);
err_mode_config_cleanup:
	// Note: drmm_mode_config_init автоматически освобождает ресурсы
err_put_device:
	if (ms912x->dmadev) {
		put_device(ms912x->dmadev);
		ms912x->dmadev = NULL;
	}
	
	// Добавляем дополнительную очистку ресурсов при ошибке
	if (ms912x->device_name[0] != '\0') {
		pr_err("ms912x: probe failed for device %s\n", ms912x->device_name);
	} else {
		pr_err("ms912x: probe failed for unknown device\n");
	}
	
	return ret;
}

static void ms912x_usb_disconnect(struct usb_interface *interface)
{
	// Добавляем проверку на NULL
	if (!interface) {
		pr_err("ms912x: invalid interface pointer\n");
		return;
	}
	
	struct ms912x_device *ms912x = usb_get_intfdata(interface);
	if (!ms912x) {
		pr_warn("ms912x: no device data found\n");
		return;
	}
	
	pr_info("ms912x: disconnect started for device %s\n", ms912x->device_name);

	struct drm_device *dev = &ms912x->drm;
	
	// Добавляем дополнительную диагностику перед отключением
	pr_info("ms912x: [%s] device state before disconnect: unplugged=%d, registered=%d\n",
	        ms912x->device_name, dev->unplugged, READ_ONCE(dev->registered));
	
	// Устанавливаем флаг отключения до выполнения других операций
	WRITE_ONCE(dev->unplugged, true);

	// Отменяем все работы
	if (cancel_work_sync(&ms912x->requests[0].work))
		pr_debug("ms912x: cancelled work [0] for device %s\n", ms912x->device_name);
		
	if (cancel_work_sync(&ms912x->requests[1].work))
		pr_debug("ms912x: cancelled work [1] for device %s\n", ms912x->device_name);

	// Завершаем работу с DRM
	drm_kms_helper_poll_fini(dev);
	
	// Принудительно завершаем все рабочие потоки перед отключением
	if (cancel_work_sync(&ms912x->requests[0].work))
		pr_debug("ms912x: [%s] cancelled work [0] during disconnect\n", ms912x->device_name);
		
	if (cancel_work_sync(&ms912x->requests[1].work))
		pr_debug("ms912x: [%s] cancelled work [1] during disconnect\n", ms912x->device_name);
	
	// Ждем завершения всех текущих операций
	wait_for_completion_timeout(&ms912x->requests[0].done, msecs_to_jiffies(1000));
	wait_for_completion_timeout(&ms912x->requests[1].done, msecs_to_jiffies(1000));
	
	// Проверяем состояние устройства перед отключением
	if (READ_ONCE(dev->registered)) {
		drm_dev_unplug(dev);
		drm_atomic_helper_shutdown(dev);
	}
	
	// Принудительно завершаем все рабочие потоки перед отключением
	if (cancel_work_sync(&ms912x->requests[0].work))
		pr_debug("ms912x: [%s] cancelled work [0] during disconnect\n", ms912x->device_name);
		
	if (cancel_work_sync(&ms912x->requests[1].work))
		pr_debug("ms912x: [%s] cancelled work [1] during disconnect\n", ms912x->device_name);
	
	// Ждем завершения всех текущих операций
	wait_for_completion_timeout(&ms912x->requests[0].done, msecs_to_jiffies(1000));
	wait_for_completion_timeout(&ms912x->requests[1].done, msecs_to_jiffies(1000));
	
	drm_dev_unplug(dev);
	drm_atomic_helper_shutdown(dev);

	// Освобождаем запросы
	ms912x_free_request(&ms912x->requests[0]);
	ms912x_free_request(&ms912x->requests[1]);

	// Освобождаем устройство DMA
	if (ms912x->dmadev) {
		put_device(ms912x->dmadev);
		ms912x->dmadev = NULL;
	}
	
	pr_info("ms912x: disconnect completed for device %s\n", ms912x->device_name);
}

static const struct usb_device_id id_table[] = {
	{ USB_DEVICE_AND_INTERFACE_INFO(0x534d, 0x6021, 0xff, 0x00, 0x00) },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x534d, 0x0821, 0xff, 0x00, 0x00) },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x345f, 0x9132, 0xff, 0x00, 0x00) },
	{},
};
MODULE_DEVICE_TABLE(usb, id_table);

// Forward declaration of the setter function
static int mode_set(const char *val, const struct kernel_param *kp);

// Parameter operations structure
static const struct kernel_param_ops mode_set_ops = {
    .set = mode_set,
    .get = NULL, // write-only
};

// Module parameter definition
module_param_cb(mode_set, &mode_set_ops, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(mode_set, "Sets the mode_num field of ms912x_mode_list[0]");

// Setter function for the mode_set parameter
static int mode_set(const char *val, const struct kernel_param *kp) {
    int ret;
    unsigned int new_value;

    ret = kstrtouint(val, 0, &new_value);
    if (ret) {
        pr_debug("ms912x: Invalid value for mode_set: %s\n", val);
        return ret;
    }

    // Update the mode_num field only for the first element
    ms912x_mode_list[0].mode = new_value;
	
    pr_debug("ms912x: ms912x_mode_list[0].mode_num set to 0x%x\n", new_value);
	
    return 0;
}

static struct usb_driver ms912x_driver = {
	.name = "ms912x",
	.probe = ms912x_usb_probe,
	.disconnect = ms912x_usb_disconnect,
	.suspend = ms912x_usb_suspend,
	.resume = ms912x_usb_resume,
	.id_table = id_table,
};
module_usb_driver(ms912x_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("USB to HDMI driver for ms912x");

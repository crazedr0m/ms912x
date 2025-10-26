#include <drm/drm_atomic_state_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_edid.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_probe_helper.h>

#include "../include/ms912x.h"

int ms912x_read_edid_block(struct ms912x_device *ms912x, u8 *buf,
				  unsigned int offset, size_t len)
{
	// Добавляем проверки на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in read_edid_block\n");
		return -EINVAL;
	}
	
	if (!buf) {
		pr_err("ms912x: invalid buffer pointer in read_edid_block\n");
		return -EINVAL;
	}
	
	if (len == 0) {
		pr_warn("ms912x: zero length requested in read_edid_block\n");
		return 0;
	}

	const u16 base = 0xc000 + offset;
	pr_debug("ms912x: reading EDID block at offset %u, len %zu\n", offset, len);
	
	for (size_t i = 0; i < len; i++) {
		u16 address = base + i;
		int ret = ms912x_read_byte(ms912x, address);
		if (ret < 0) {
			pr_err("ms912x: failed to read EDID byte at 0x%04x: %d\n", address, ret);
			return ret;
		}
		buf[i] = ret;
	}
	
	pr_debug("ms912x: successfully read %zu bytes from EDID\n", len);
	
	// Добавляем дополнительную диагностику при успешном чтении
	if (len >= 8) {
		pr_debug("ms912x: [%s] EDID header: %*ph\n",
		         ms912x->device_name, 8, buf);
	}
	
	return 0;
}

static int ms912x_read_edid(void *data, u8 *buf, unsigned int block, size_t len)
{
	// Добавляем проверки на NULL
	if (!data) {
		pr_err("ms912x: invalid data pointer in read_edid\n");
		return -EINVAL;
	}
	
	if (!buf) {
		pr_err("ms912x: invalid buffer pointer in read_edid\n");
		return -EINVAL;
	}
	
	struct ms912x_device *ms912x = data;
	const int offset = block * EDID_LENGTH;
	
	pr_debug("ms912x: reading EDID block %u, offset %d, len %zu\n", block, offset, len);
	
	int ret = ms912x_read_edid_block(ms912x, buf, offset, len);
	if (ret < 0) {
		pr_err("ms912x: failed to read EDID block %u: %d\n", block, ret);
		return ret;
	}
	
	pr_debug("ms912x: successfully read EDID block %u\n", block);
	return ret;
}

static void ms912x_add_fallback_mode(struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

	mode->clock = 65000;
	mode->hdisplay = 1024;
	mode->hsync_start = 1048;
	mode->hsync_end = 1184;
	mode->htotal = 1344;
	mode->vdisplay = 768;
	mode->vsync_start = 771;
	mode->vsync_end = 777;
	mode->vtotal = 806;
	mode->flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;

	drm_mode_probed_add(connector, mode);
	
	// Добавляем дополнительную диагностику при добавлении резервного режима
	if (mode->name) {
		pr_info("ms912x: added fallback mode: %s %dx%d@%dHz\n",
		        mode->name, mode->hdisplay, mode->vdisplay, drm_mode_vrefresh(mode));
	}
}

static int ms912x_connector_get_modes(struct drm_connector *connector)
{
	// Добавляем проверку на NULL
	if (!connector) {
		pr_err("ms912x: invalid connector pointer in get_modes\n");
		return -EINVAL;
	}
	
	int ret = 0;
	struct ms912x_device *ms912x = to_ms912x(connector->dev);
	const struct drm_edid *edid;

	pr_debug("ms912x: reading EDID information\n");
	
	edid = drm_edid_read_custom(connector, ms912x_read_edid, ms912x);
	if (!edid) {
		pr_warn("ms912x: EDID not found, falling back to default mode\n");
		ms912x_add_fallback_mode(connector);
		return 1;
	}

	pr_debug("ms912x: EDID read successfully, updating connector\n");
	pr_info("ms912x: [%s] EDID read successfully, updating connector with EDID data\n",
	        ms912x->device_name);
	
	ret = drm_edid_connector_update(connector, edid);
	if (ret < 0) {
		pr_err("ms912x: failed to update EDID connector: %d\n", ret);
		ret = 0;
		goto edid_free;
	}

	pr_debug("ms912x: adding modes from EDID\n");
	ret = drm_edid_connector_add_modes(connector);
	
	pr_info("ms912x: added %d modes from EDID\n", ret);
	
	// Выводим список видеорежимов, поддерживаемых монитором
	if (ret > 0) {
		struct drm_display_mode *mode;
		int mode_count = 0;
		
		pr_info("ms912x: monitor supported video modes:\n");
		list_for_each_entry(mode, &connector->modes, head) {
			pr_info("ms912x: mode %d: %dx%d@%dHz flags=0x%x type=0x%x\n",
			        mode_count, mode->hdisplay, mode->vdisplay,
			        drm_mode_vrefresh(mode), mode->flags, mode->type);
			mode_count++;
		}
		pr_info("ms912x: total monitor modes: %d\n", mode_count);
	}

edid_free:
	drm_edid_free(edid);
	return ret;
}

static enum drm_connector_status ms912x_detect(struct drm_connector *connector,
					       bool force)
{
	// Добавляем проверку на NULL
	if (!connector) {
		pr_err("ms912x: invalid connector pointer in detect\n");
		return connector_status_unknown;
	}
	
	struct ms912x_device *ms912x = to_ms912x(connector->dev);
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in detect\n");
		return connector_status_unknown;
	}
	
	pr_debug("ms912x: detecting HDMI status\n");
	
	int status = ms912x_read_byte(ms912x, 0x32);
	pr_debug("ms912x: HDMI status register value: %d\n", status);

	if (status < 0) {
		pr_err("ms912x: failed to detect HDMI status: %d\n", status);
		return connector_status_unknown;
	}

	enum drm_connector_status result = (status == 1) ? connector_status_connected :
			       connector_status_disconnected;
			       
	pr_debug("ms912x: detect result: %s\n",
		result == connector_status_connected ? "connected" : "disconnected");
		
	// Добавляем дополнительную диагностику после обнаружения
	pr_info("ms912x: [%s] HDMI detection result: %s (status register: %d)\n",
	        ms912x->device_name,
	        result == connector_status_connected ? "connected" : "disconnected",
	        status);
		
	return result;
}

static const struct drm_connector_helper_funcs ms912x_connector_helper_funcs = {
	.get_modes = ms912x_connector_get_modes,
};

static const struct drm_connector_funcs ms912x_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.detect = ms912x_detect,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

int ms912x_connector_init(struct ms912x_device *ms912x)
{
	int ret;

	pr_info("ms912x: [%s] initializing connector\n", ms912x->device_name);
	
	drm_connector_helper_add(&ms912x->connector,
				 &ms912x_connector_helper_funcs);

	ret = drm_connector_init(&ms912x->drm, &ms912x->connector,
				 &ms912x_connector_funcs,
				 DRM_MODE_CONNECTOR_HDMIA);
	if (ret) {
		pr_err("ms912x: [%s] failed to initialize connector: %d\n",
		       ms912x->device_name, ret);
		return ret;
	}
	
	pr_info("ms912x: [%s] connector initialized successfully\n", ms912x->device_name);

	ms912x->connector.polled =
		DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	pr_info("ms912x: [%s] connector polling enabled: CONNECT|DISCONNECT\n",
	        ms912x->device_name);

	return 0;
}

#include <uapi/linux/hid.h>
#include "../include/ms912x.h"

int ms912x_read_byte(struct ms912x_device *ms912x, u16 address)
{
	// Добавляем проверку на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in read_byte\n");
		return -EINVAL;
	}
	
	int ret;
	struct usb_interface *intf = ms912x->intf;
	if (!intf) {
		pr_err("ms912x: invalid interface pointer in read_byte\n");
		return -EINVAL;
	}
	
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		pr_err("ms912x: invalid usb device pointer in read_byte\n");
		return -EINVAL;
	}
	
	pr_debug("ms912x: reading byte from address 0x%04x\n", address);
	
	struct ms912x_request *request = kzalloc(8, GFP_KERNEL);
	if (!request) {
		pr_err("ms912x: failed to allocate request in read_byte\n");
		return -ENOMEM;
	}

	request->type = 0xb5;
	request->addr = cpu_to_be16(address);

	ret = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			HID_REQ_SET_REPORT,
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			0x0300, 0, request, 8, USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		pr_err("ms912x: failed to send read request to address 0x%04x: %d\n", address, ret);
		kfree(request);
		return ret;
	}

	ret = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			      HID_REQ_GET_REPORT,
			      USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			      0x0300, 0, request, 8, USB_CTRL_GET_TIMEOUT);
	if (ret < 0) {
		pr_err("ms912x: failed to receive read response from address 0x%04x: %d\n", address, ret);
		kfree(request);
		return ret;
	}

	ret = (ret > 0) ? request->data[0] : (ret == 0) ? -EIO : ret;
	
	pr_debug("ms912x: read byte from address 0x%04x: 0x%02x\n", address, ret);

	kfree(request);
	return ret;
}

static inline int ms912x_write_6_bytes(struct ms912x_device *ms912x,
				       u16 address, void *data)
{
	// Добавляем проверки на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in write_6_bytes\n");
		return -EINVAL;
	}
	
	if (!data) {
		pr_err("ms912x: invalid data pointer in write_6_bytes\n");
		return -EINVAL;
	}
	
	struct usb_interface *intf = ms912x->intf;
	if (!intf) {
		pr_err("ms912x: invalid interface pointer in write_6_bytes\n");
		return -EINVAL;
	}
	
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	if (!usb_dev) {
		pr_err("ms912x: invalid usb device pointer in write_6_bytes\n");
		return -EINVAL;
	}
	
	pr_debug("ms912x: writing 6 bytes to address 0x%04x\n", address);
	
	struct ms912x_write_request *request =
		kzalloc(sizeof(struct ms912x_write_request), GFP_KERNEL);
	if (!request) {
		pr_err("ms912x: failed to allocate write request\n");
		return -ENOMEM;
	}

	request->type = 0xa6;
	request->addr = address;
	memcpy(request->data, data, 6);

	int ret = usb_control_msg(
		usb_dev, usb_sndctrlpipe(usb_dev, 0), HID_REQ_SET_REPORT,
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0x0300, 0,
		request, sizeof(*request), USB_CTRL_SET_TIMEOUT);
	
	if (ret < 0) {
		pr_err("ms912x: failed to write 6 bytes to address 0x%04x: %d\n", address, ret);
	} else {
		pr_debug("ms912x: successfully wrote 6 bytes to address 0x%04x\n", address);
	}

	kfree(request);
	return ret;
}

int ms912x_power_on(struct ms912x_device *ms912x)
{
	// Добавляем проверку на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in power_on\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: powering on device\n");
	
	u8 data[6] = { 0x01, 0x02 };
	int ret = ms912x_write_6_bytes(ms912x, 0x07, data);
	
	if (ret < 0) {
		pr_err("ms912x: failed to power on device: %d\n", ret);
	} else {
		pr_info("ms912x: device powered on successfully\n");
	}
	
	return ret;
}

int ms912x_power_off(struct ms912x_device *ms912x)
{
	// Добавляем проверку на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in power_off\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: powering off device\n");
	
	u8 data[6] = { 0 };
	int ret = ms912x_write_6_bytes(ms912x, 0x07, data);
	
	if (ret < 0) {
		pr_err("ms912x: failed to power off device: %d\n", ret);
	} else {
		pr_info("ms912x: device powered off successfully\n");
	}
	
	return ret;
}

int ms912x_set_resolution(struct ms912x_device *ms912x,
			  const struct ms912x_mode *mode)
{
	// Добавляем проверки на NULL
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in set_resolution\n");
		return -EINVAL;
	}
	
	if (!mode) {
		pr_err("ms912x: invalid mode pointer in set_resolution\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: setting resolution %dx%d, mode 0x%04x\n",
		mode->width, mode->height, mode->mode);
	
	u8 data[6] = { 0 };
	int ret;

	pr_debug("ms912x: step 1 - reset display\n");
	ret = ms912x_write_6_bytes(ms912x, 0x04, data);
	if (ret < 0) {
		pr_err("ms912x: failed to reset display: %d\n", ret);
		return ret;
	}

	pr_debug("ms912x: step 2 - read status registers\n");
	ms912x_read_byte(ms912x, 0x30);
	ms912x_read_byte(ms912x, 0x33);
	ms912x_read_byte(ms912x, 0xc620);

	pr_debug("ms912x: step 3 - set configuration mode\n");
	data[0] = 0x03;
	ret = ms912x_write_6_bytes(ms912x, 0x03, data);
	if (ret < 0) {
		pr_err("ms912x: failed to set configuration mode: %d\n", ret);
		return ret;
	}

	pr_debug("ms912x: step 4 - set resolution\n");
	struct ms912x_resolution_request resolution_request = {
		.width = cpu_to_be16(mode->width),
		.height = cpu_to_be16(mode->height),
		.pixel_format = cpu_to_be16(mode->pix_fmt)
	};
	ret = ms912x_write_6_bytes(ms912x, 0x01, &resolution_request);
	if (ret < 0) {
		pr_err("ms912x: failed to set resolution: %d\n", ret);
		return ret;
	}

	pr_debug("ms912x: step 5 - set mode\n");
	struct ms912x_mode_request mode_request = {
		.mode = cpu_to_be16(mode->mode),
		.width = cpu_to_be16(mode->width),
		.height = cpu_to_be16(mode->height)
	};
	ret = ms912x_write_6_bytes(ms912x, 0x02, &mode_request);
	if (ret < 0) {
		pr_err("ms912x: failed to set mode: %d\n", ret);
		return ret;
	}

	pr_debug("ms912x: step 6 - enable display\n");
	data[0] = 1;
	ret = ms912x_write_6_bytes(ms912x, 0x04, data);
	if (ret < 0) {
		pr_err("ms912x: failed to enable display: %d\n", ret);
		return ret;
	}

	pr_debug("ms912x: step 7 - final configuration\n");
	/* Same data reused here */
	ret = ms912x_write_6_bytes(ms912x, 0x05, data);
	if (ret < 0) {
		pr_err("ms912x: failed final configuration: %d\n", ret);
		return ret;
	}
	
	pr_info("ms912x: resolution set successfully\n");
	return 0;
}

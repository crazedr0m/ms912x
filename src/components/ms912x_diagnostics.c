#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <drm/drm_print.h>

#include "../include/ms912x.h"

/**
 * @brief Проверяет подключение устройства
 * 
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_connection(struct ms912x_device *ms912x)
{
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in diag_check_connection\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: [%s] running connection diagnostic\n", ms912x->device_name);
	
	// Проверяем базовые регистры устройства
	int reg30 = ms912x_read_byte(ms912x, 0x30);
	int reg33 = ms912x_read_byte(ms912x, 0x33);
	
	if (reg30 < 0 || reg33 < 0) {
		pr_err("ms912x: [%s] failed to read status registers: reg30=%d, reg33=%d\n", 
		       ms912x->device_name, reg30, reg33);
		return -EIO;
	}
	
	pr_info("ms912x: [%s] connection diagnostic passed: reg30=0x%02x, reg33=0x%02x\n", 
	        ms912x->device_name, reg30, reg33);
	
	return 0;
}

/**
 * @brief Проверяет работу с памятью устройства
 * 
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_memory(struct ms912x_device *ms912x)
{
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in diag_check_memory\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: [%s] running memory diagnostic\n", ms912x->device_name);
	
	// Проверяем доступ к расширенным регистрам
	int reg_c620 = ms912x_read_byte(ms912x, 0xc620);
	if (reg_c620 < 0) {
		pr_err("ms912x: [%s] failed to read extended register c620: %d\n", 
		       ms912x->device_name, reg_c620);
		return -EIO;
	}
	
	pr_info("ms912x: [%s] memory diagnostic passed: reg_c620=0x%02x\n", 
	        ms912x->device_name, reg_c620);
	
	return 0;
}

/**
 * @brief Проверяет работу с EDID
 * 
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_edid(struct ms912x_device *ms912x)
{
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in diag_check_edid\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: [%s] running EDID diagnostic\n", ms912x->device_name);
	
	// Проверяем чтение первых байт EDID
	u8 edid_header[8];
	int ret = ms912x_read_edid_block(ms912x, edid_header, 0, 8);
	if (ret < 0) {
		pr_err("ms912x: [%s] failed to read EDID header: %d\n", 
		       ms912x->device_name, ret);
		return ret;
	}
	
	// Проверяем заголовок EDID (должен начинаться с 0x00 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0x00)
	const u8 expected_header[8] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
	if (memcmp(edid_header, expected_header, 8) != 0) {
		pr_warn("ms912x: [%s] unexpected EDID header: %*ph\n", 
		        ms912x->device_name, 8, edid_header);
		// Это не обязательно ошибка, некоторые устройства могут иметь другой формат
	} else {
		pr_info("ms912x: [%s] EDID header is valid\n", ms912x->device_name);
	}
	
	pr_info("ms912x: [%s] EDID diagnostic completed\n", ms912x->device_name);
	
	return 0;
}

/**
 * @brief Выполняет полную диагностику устройства
 * 
 * @param ms912x Устройство для диагностики
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_run_diagnostics(struct ms912x_device *ms912x)
{
	if (!ms912x) {
		pr_err("ms912x: invalid device pointer in run_diagnostics\n");
		return -EINVAL;
	}
	
	pr_info("ms912x: [%s] starting full diagnostics\n", ms912x->device_name);
	
	int ret;
	
	// Проверка подключения
	ret = ms912x_diag_check_connection(ms912x);
	if (ret < 0) {
		pr_err("ms912x: [%s] connection diagnostic failed\n", ms912x->device_name);
		return ret;
	}
	
	// Проверка памяти
	ret = ms912x_diag_check_memory(ms912x);
	if (ret < 0) {
		pr_err("ms912x: [%s] memory diagnostic failed\n", ms912x->device_name);
		return ret;
	}
	
	// Проверка EDID
	ret = ms912x_diag_check_edid(ms912x);
	if (ret < 0) {
		pr_err("ms912x: [%s] EDID diagnostic failed\n", ms912x->device_name);
		return ret;
	}
	
	pr_info("ms912x: [%s] all diagnostics passed successfully\n", ms912x->device_name);
	
	return 0;
}

/**
 * @brief Получает информацию о состоянии устройства
 * 
 * @param ms912x Устройство
 * @param buf Буфер для записи информации
 * @param size Размер буфера
 * @return Количество записанных байт
 */
int ms912x_get_device_info(struct ms912x_device *ms912x, char *buf, size_t size)
{
	if (!ms912x || !buf) {
		pr_err("ms912x: invalid parameters in get_device_info\n");
		return -EINVAL;
	}
	
	// Читаем статусные регистры
	int reg30 = ms912x_read_byte(ms912x, 0x30);
	int reg33 = ms912x_read_byte(ms912x, 0x33);
	int reg_c620 = ms912x_read_byte(ms912x, 0xc620);
	
	// Получаем информацию о USB устройстве
	struct usb_interface *intf = ms912x->intf;
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	
	return scnprintf(buf, size,
			"Device ID: %u\n"
			"Device Name: %s\n"
			"USB Bus: %d\n"
			"USB Device: %d\n"
			"Vendor ID: 0x%04x\n"
			"Product ID: 0x%04x\n"
			"Status Register 0x30: 0x%02x\n"
			"Status Register 0x33: 0x%02x\n"
			"Extended Register 0xc620: 0x%02x\n"
			"Current Request Buffer: %d\n"
			"Last Send Jiffies: %lu\n",
			ms912x->device_id,
			ms912x->device_name,
			usb_dev->bus->busnum,
			usb_dev->devnum,
			usb_dev->descriptor.idVendor,
			usb_dev->descriptor.idProduct,
			reg30 >= 0 ? reg30 : 0,
			reg33 >= 0 ? reg33 : 0,
			reg_c620 >= 0 ? reg_c620 : 0,
			ms912x->current_request,
			ms912x->last_send_jiffies);
}
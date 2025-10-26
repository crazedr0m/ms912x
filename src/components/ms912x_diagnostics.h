#ifndef MS912X_DIAGNOSTICS_H
#define MS912X_DIAGNOSTICS_H

#include <linux/types.h>

// Forward declaration of ms912x_device struct
struct ms912x_device;

/**
 * @brief Проверяет подключение устройства
 *
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_connection(struct ms912x_device *ms912x);

/**
 * @brief Проверяет работу с памятью устройства
 *
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_memory(struct ms912x_device *ms912x);

/**
 * @brief Проверяет работу с EDID
 *
 * @param ms912x Устройство для проверки
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_diag_check_edid(struct ms912x_device *ms912x);

/**
 * @brief Выполняет полную диагностику устройства
 *
 * @param ms912x Устройство для диагностики
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ms912x_run_diagnostics(struct ms912x_device *ms912x);

/**
 * @brief Получает информацию о состоянии устройства
 *
 * @param ms912x Устройство
 * @param buf Буфер для записи информации
 * @param size Размер буфера
 * @return Количество записанных байт
 */
int ms912x_get_device_info(struct ms912x_device *ms912x, char *buf, size_t size);


#endif // MS912X_DIAGNOSTICS_H
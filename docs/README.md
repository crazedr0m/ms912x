# ms912x driver for Linux (kernel 6.8 beta)

Linux kernel driver for MacroSilicon USB to VGA/HDMI adapter. VID/PID is 534d:6021. Device is USB2.0

There are two variants:
 - VID/PID is 534d:6021. Device is USB 2
 - VID/PID is 345f:9132. Device is USB 3

- Driver adapted for Linux kernel 6.8 by dcomp7
- Improved performance.
- Fixed DRM_COPY_FIELD error
- Enhanced error handling and logging

For kernel 6.15 checkout branch kernel-6.15
For kernel 6.10,6.12,6.11 checkout branch kernel-6.10

## Project Structure

```
ms912x/
├── src/                    # Основные исходные файлы драйвера
│   ├── core/              # Основные компоненты драйвера
│   │   ├── ms912x_drv.c
│   │   └── ms912x.mod.c
│   ├── components/        # Компоненты драйвера
│   │   ├── ms912x_connector.c
│   │   ├── ms912x_registers.c
│   │   ├── ms912x_transfer.c
│   │   ├── ms912x_diagnostics.c
│   │   └── ms912x_diagnostics.h
│   └── include/           # Заголовочные файлы
│       └── ms912x.h
├── docs/                  # Документация
│   ├── README.md
│   └── INSTALL.md
├── scripts/               # Скрипты сборки и установки
│   ├── Makefile
│   └── dkms.conf
├── tests/                 # Тесты и инструменты диагностики
├── LICENSE                # Лицензия
└── .clang-format
```

## Installation

See INSTALL.md for detailed installation instructions.

## Development

Driver is written by analyzing wireshark captures of the device.

### Building the driver

To build the driver, simply run:

```bash
make
```

This will generate the following files:
- `ms912x.o` - The driver object file
- `ms912x.ko` - The kernel module
- `ms912x.mod.c` - Module source file
- `ms912x.mod.o` - Module object file

### Cleaning the build

To clean the build files, run:

```bash
make clean
```

## DKMS

Run `sudo dkms install .`

- make clean
- make
- sudo rmmod ms912x # It will not work if the device is in use.
- sudo modprobe drm_shmem_helper
- sudo insmod ms912x.ko

Forked From: https://github.com/rhgndf/ms912x

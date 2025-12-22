// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <efi/payload.h>
#include <efi/error.h>
#include <efi/payload/driver.h>
#include <efi/protocol/serial.h>

/*
 * We wrap our port structure around the generic console_device.
 */
struct efi_serial_port {
	struct efi_serial_io_protocol *serial;
	struct console_device	uart;		/* uart */
	struct efi_device *efidev;
};

static inline struct efi_serial_port *
to_efi_serial_port(struct console_device *uart)
{
	return container_of(uart, struct efi_serial_port, uart);
}

static int efi_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	efi_status_t efiret;

	efiret = serial->set_attributes(serial, baudrate, 0, 0, NoParity, 8,
					OneStopBit);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static void efi_serial_putc(struct console_device *cdev, char c)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	uint32_t control;
	efi_status_t efiret;
	size_t buffersize = sizeof(char);

	do {
		efiret = serial->getcontrol(serial, &control);
		if (EFI_ERROR(efiret))
			return;

	} while(!(control & EFI_SERIAL_CLEAR_TO_SEND));

	serial->write(serial, &buffersize, &c);
}

static int efi_serial_puts(struct console_device *cdev, const char *s,
			   size_t nbytes)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	uint32_t control;
	efi_status_t efiret;
	size_t buffersize = nbytes;

	do {
		efiret = serial->getcontrol(serial, &control);
		if (EFI_ERROR(efiret))
			return 0;

	} while(!(control & EFI_SERIAL_CLEAR_TO_SEND));

	serial->write(serial, &buffersize, (void*)s);

	return strlen(s);
}

static int efi_serial_getc(struct console_device *cdev)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	uint32_t control;
	efi_status_t efiret;
	size_t buffersize = sizeof(char);
	char c;

	do {
		efiret = serial->getcontrol(serial, &control);
		if (EFI_ERROR(efiret))
			return (int)-1;

	} while(!(control & EFI_SERIAL_DATA_SET_READY));

	serial->read(serial, &buffersize, &c);

	return (int)c;
}

static int efi_serial_tstc(struct console_device *cdev)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	uint32_t control;
	efi_status_t efiret;

	efiret = serial->getcontrol(serial, &control);
	if (EFI_ERROR(efiret))
		return 0;

	return !(control & EFI_SERIAL_INPUT_BUFFER_EMPTY);
}

static int efi_serial_probe(struct efi_device *efidev)
{
	struct efi_serial_port *uart;
	struct console_device *cdev;

	uart = xzalloc(sizeof(struct efi_serial_port));

	cdev = &uart->uart;
	cdev->dev = &efidev->dev;
	cdev->tstc = efi_serial_tstc;
	cdev->putc = efi_serial_putc;
	cdev->puts = efi_serial_puts;
	cdev->getc = efi_serial_getc;
	cdev->setbrg = efi_serial_setbaudrate;

	uart->serial = efidev->protocol;

	uart->serial->reset(uart->serial);

	/* Enable UART */

	console_register(cdev);

	return 0;
}

static struct efi_driver efi_serial_driver = {
        .driver = {
		.name  = "efi-serial",
	},
        .probe = efi_serial_probe,
	.guid = EFI_SERIAL_IO_PROTOCOL_GUID,
};
device_efi_driver(efi_serial_driver);

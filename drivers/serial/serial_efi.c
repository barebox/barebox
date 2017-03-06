/*
 * Copyright (C) 2017 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 Only
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <efi.h>
#include <efi/efi.h>
#include <efi/efi-device.h>

/*
 * define for Control bits, grouped by read only, write only, and read write
 *
 * Read Only
 */
#define EFI_SERIAL_CLEAR_TO_SEND        0x00000010
#define EFI_SERIAL_DATA_SET_READY       0x00000020
#define EFI_SERIAL_RING_INDICATE        0x00000040
#define EFI_SERIAL_CARRIER_DETECT       0x00000080
#define EFI_SERIAL_INPUT_BUFFER_EMPTY   0x00000100
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY  0x00000200

/*
 * Write Only
 */
#define EFI_SERIAL_REQUEST_TO_SEND      0x00000002
#define EFI_SERIAL_DATA_TERMINAL_READY  0x00000001

/*
 * Read Write
 */
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE     0x00001000
#define EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE     0x00002000
#define EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE 0x00004000

typedef enum {
	DefaultParity,
	NoParity,
	EvenParity,
	OddParity,
	MarkParity,
	SpaceParity
} efi_parity_type;

typedef enum {
	DefaultStopBits,
	OneStopBit,
	OneFiveStopBits,
	TwoStopBits
} efi_stop_bits_type;

struct efi_serial_io_mode {
	uint32_t controlmask;
	uint32_t timeout;
	uint64_t baudrate;
	uint32_t receivefifodepth;
	uint32_t databits;
	uint32_t parity;
	uint32_t stopbits;
};

struct efi_serial_io_protocol {
	uint32_t revision;

	efi_status_t (EFIAPI *reset) (struct efi_serial_io_protocol *This);
	efi_status_t (EFIAPI *set_attributes) (struct efi_serial_io_protocol *This,
			uint64_t baudrate, uint32_t receivefifodepth,
			uint32_t timeout, efi_parity_type parity,
			uint8_t databits, efi_stop_bits_type stopbits);
	efi_status_t (EFIAPI *setcontrol) (struct efi_serial_io_protocol *This,
			uint32_t control);
	efi_status_t (EFIAPI *getcontrol) (struct efi_serial_io_protocol *This,
			uint32_t *control);
	efi_status_t (EFIAPI *write) (struct efi_serial_io_protocol *This,
			unsigned long *buffersize, void *buffer);
	efi_status_t (EFIAPI *read) (struct efi_serial_io_protocol *This,
			unsigned long *buffersize, void *buffer);

	struct efi_serial_io_mode *mode;
};

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
	unsigned long buffersize = sizeof(char);

	do {
		efiret = serial->getcontrol(serial, &control);
		if (EFI_ERROR(efiret))
			return;

	} while(!(control & EFI_SERIAL_CLEAR_TO_SEND));

	serial->write(serial, &buffersize, &c);
}

static int efi_serial_puts(struct console_device *cdev, const char *s)
{
	struct efi_serial_port *uart = to_efi_serial_port(cdev);
	struct efi_serial_io_protocol *serial = uart->serial;
	uint32_t control;
	efi_status_t efiret;
	unsigned long buffersize = strlen(s) * sizeof(char);

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
	unsigned long buffersize = sizeof(char);
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

/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_SPI_H
#define __INCLUDE_SPI_H

#include <driver.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/string.h>

struct spi_controller_mem_ops;

struct spi_board_info {
	char	*name;
	int 	max_speed_hz;
	int	bus_num;
	int	chip_select;

	/* mode becomes spi_device.mode, and is essential for chips
	 * where the default of SPI_CS_HIGH = 0 is wrong.
	 */
	u8	mode;
	u8	bits_per_word;
	void	*platform_data;
	struct device_node *device_node;
};

/**
 * struct spi_device - Controller side proxy for an SPI slave device
 * @dev: Driver model representation of the device.
 * @controller: SPI controller used with the device.
 * @master: Copy of controller, for backwards compatibility
 * @max_speed_hz: Maximum clock rate to be used with this chip
 *	(on this board); may be changed by the device's driver.
 *	The spi_transfer.speed_hz can override this for each transfer.
 * @chip_select: Chipselect, distinguishing chips handled by @controller.
 * @mode: The spi mode defines how data is clocked out and in.
 *	This may be changed by the device's driver.
 *	The "active low" default for chipselect mode can be overridden
 *	(by specifying SPI_CS_HIGH) as can the "MSB first" default for
 *	each word in a transfer (by specifying SPI_LSB_FIRST).
 * @bits_per_word: Data transfers involve one or more words; word sizes
 *	like eight or 12 bits are common.  In-memory wordsizes are
 *	powers of two bytes (e.g. 20 bit samples use 32 bits).
 *	This may be changed by the device's driver, or left at the
 *	default (0) indicating protocol words are eight bit bytes.
 *	The spi_transfer.bits_per_word can override this for each transfer
 *	(FIXME: not currently implemented).
 * @irq: Negative, or the number passed to request_irq() to receive
 *	interrupts from this device.
 * @controller_state: Controller's runtime state
 * @controller_data: Board-specific definitions for controller, such as
 *	FIFO initialization parameters; from board_info.controller_data
 * @modalias: Name of the driver to use with this device, or an alias
 *	for that name.  This appears in the sysfs "modalias" attribute
 *	for driver coldplugging, and in uevents used for hotplugging
 *
 * A @spi_device is used to interchange data between an SPI slave
 * (usually a discrete chip) and CPU memory.
 *
 * In @dev, the platform_data is used to hold information about this
 * device that's meaningful to the device's protocol driver, but not
 * to its controller.  One example might be an identifier for a chip
 * variant with slightly different functionality; another might be
 * information about how this particular board wires the chip's pins.
 */
struct spi_device {
	struct device_d		dev;
	struct spi_controller	*controller;
	struct spi_controller	*master;	/* compatibility layer */
	struct spi_mem		*mem;
	u32			max_speed_hz;
	u8			chip_select;
	u8			mode;
#define	SPI_CPHA	0x01			/* clock phase */
#define	SPI_CPOL	0x02			/* clock polarity */
#define	SPI_MODE_0	(0|0)			/* (original MicroWire) */
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define	SPI_CS_HIGH	0x04			/* chipselect active high? */
#define	SPI_LSB_FIRST	0x08			/* per-word bits-on-wire */
#define	SPI_3WIRE	0x10			/* SI/SO signals shared */
#define	SPI_LOOP	0x20			/* loopback mode */
#define	SPI_NO_CS	0x40			/* 1 dev/bus, no chipselect */
#define	SPI_READY	0x80			/* slave pulls low to pause */
#define	SPI_TX_DUAL	0x100			/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200			/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400			/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800			/* receive with 4 wires */
#define	SPI_CS_WORD	0x1000			/* toggle cs after each word */
#define	SPI_TX_OCTAL	0x2000			/* transmit with 8 wires */
#define	SPI_RX_OCTAL	0x4000			/* receive with 8 wires */
#define	SPI_3WIRE_HIZ	0x8000			/* high impedance turnaround */
	u8			bits_per_word;
	int			irq;
	void			*controller_state;
	void			*controller_data;
	const char		*modalias;

	/*
	 * likely need more hooks for more protocol options affecting how
	 * the controller talks to each chip, like:
	 *  - memory packing (12 bit samples into low bits, others zeroed)
	 *  - priority
	 *  - drop chipselect after each word
	 *  - chipselect delays
	 *  - ...
	 */
};

struct spi_message;

/**
 * struct spi_controller - interface to SPI master or slave controller
 * @dev: device interface to this driver
 * @bus_num: board-specific (and often SOC-specific) identifier for a
 *	given SPI controller.
 * @mem_ops: optimized/dedicated operations for interactions with SPI
 *      memory. This field is optional and should only be implemented
 *      if the controller has native support for memory like operations.
 * @max_transfer_size: function that returns the max transfer size for
 *	a &spi_device; may be %NULL, so the default %SIZE_MAX will be used.
 * @max_message_size: function that returns the max message size for
 *	a &spi_device; may be %NULL, so the default %SIZE_MAX will be used.
 * @num_chipselect: chipselects are used to distinguish individual
 *	SPI slaves, and are numbered from zero to num_chipselects.
 *	each slave has a chipselect signal, but it's common that not
 *	every chipselect is connected to a slave.
 * @max_speed_hz: Highest supported transfer speed
 * @setup: updates the device mode and clocking records used by a
 *	device's SPI controller; protocol code may call this.  This
 *	must fail if an unrecognized or unsupported mode is requested.
 *	It's always safe to call this unless transfers are pending on
 *	the device whose settings are being modified.
 * @transfer: adds a message to the controller's transfer queue.
 * @cleanup: frees controller-specific state
 * @list: link with the global spi_controller list
 *
 * Each SPI controller can communicate with one or more @spi_device
 * children.  These make a small bus, sharing MOSI, MISO and SCK signals
 * but not chip select signals.  Each device may be configured to use a
 * different clock rate, since those shared signals are ignored unless
 * the chip is selected.
 *
 * The driver for an SPI controller manages access to those devices through
 * a queue of spi_message transactions, copying data between CPU memory and
 * an SPI slave device.  For each such message it queues, it calls the
 * message's completion function when the transaction completes.
 */
struct spi_controller {
	struct device_d *dev;

	/* other than negative (== assign one dynamically), bus_num is fully
	 * board-specific.  usually that simplifies to being SOC-specific.
	 * example:  one SOC has three SPI controllers, numbered 0..2,
	 * and one board's schematics might show it using SPI-2.  software
	 * would normally use bus_num=2 for that controller.
	 */
	s16			bus_num;

	/* Optimized handlers for SPI memory-like operations */
	const struct spi_controller_mem_ops *mem_ops;
	/*
	 * on some hardware transfer size may be constrained
	 * the limit may depend on device transfer settings
	 */
	size_t (*max_transfer_size)(struct spi_device *spi);
	size_t (*max_message_size)(struct spi_device *spi);

	/* chipselects will be integral to many controllers; some others
	 * might use board-specific GPIOs.
	 */
	u16			num_chipselect;

	/* limits on transfer speed */
	u32			max_speed_hz;

	/* setup mode and clock, etc (spi driver may call many times) */
	int			(*setup)(struct spi_device *spi);

	/* bidirectional bulk transfers
	 *
	 * + The transfer() method may not sleep; its main role is
	 *   just to add the message to the queue.
	 * + For now there's no remove-from-queue operation, or
	 *   any other request management
	 * + To a given spi_device, message queueing is pure fifo
	 *
	 * + The controller's main job is to process its message queue,
	 *   selecting a chip then transferring data
	 * + If there are multiple spi_device children, the i/o queue
	 *   arbitration algorithm is unspecified (round robin, fifo,
	 *   priority, reservations, preemption, etc)
	 *
	 * + Chipselect stays active during the entire message
	 *   (unless modified by spi_transfer.cs_change != 0).
	 * + The message transfers use clock and SPI mode parameters
	 *   previously established by setup() for this device
	 */
	int			(*transfer)(struct spi_device *spi,
						struct spi_message *mesg);

	/* called on release() to free memory provided by spi_controller */
	void			(*cleanup)(struct spi_device *spi);

	struct list_head list;
};

static inline void *spi_controller_get_devdata(struct spi_controller *ctlr)
{
	if (ctlr->dev->platform_data)
		return ctlr->dev->platform_data;
	else
		return ERR_PTR(-EINVAL);
}

static inline void spi_controller_set_devdata(struct spi_controller *ctlr,
					      void *data)
{
	ctlr->dev->platform_data = data;
}

static inline size_t spi_max_message_size(struct spi_device *spi)
{
	struct spi_controller *ctrl = spi->controller;
	if (!ctrl->max_transfer_size)
		return SIZE_MAX;
	return ctrl->max_transfer_size(spi);
}

static inline size_t spi_max_transfer_size(struct spi_device *spi)
{
	struct spi_controller *ctrl = spi->controller;
	size_t tr_max = SIZE_MAX;
	size_t msg_max = spi_max_message_size(spi);

	if (ctrl->max_transfer_size)
		tr_max = ctrl->max_transfer_size(spi);

	return min(tr_max, msg_max);
}

#define spi_master  			spi_controller
#define spi_register_master(_ctrl)	spi_register_controller(_ctrl)

/*---------------------------------------------------------------------------*/

/*
 * I/O INTERFACE between SPI controller and protocol drivers
 *
 * Protocol drivers use a queue of spi_messages, each transferring data
 * between the controller and memory buffers.
 *
 * The spi_messages themselves consist of a series of read+write transfer
 * segments.  Those segments always read the same number of bits as they
 * write; but one or the other is easily ignored by passing a null buffer
 * pointer.  (This is unlike most types of I/O API, because SPI hardware
 * is full duplex.)
 *
 * NOTE:  Allocation of spi_transfer and spi_message memory is entirely
 * up to the protocol driver, which guarantees the integrity of both (as
 * well as the data buffers) for as long as the message is queued.
 */

/**
 * struct spi_transfer - a read/write buffer pair
 * @tx_buf: data to be written (dma-safe memory), or NULL
 * @rx_buf: data to be read (dma-safe memory), or NULL
 * @len: size of rx and tx buffers (in bytes)
 * @speed_hz: Select a speed other then the device default for this
 *      transfer. If 0 the default (from @spi_device) is used.
 * @bits_per_word: select a bits_per_word other then the device default
 *      for this transfer. If 0 the default (from @spi_device) is used.
 * @cs_change: affects chipselect after this transfer completes
 * @delay_usecs: microseconds to delay after this transfer before
 *	(optionally) changing the chipselect status, then starting
 *	the next transfer or completing this @spi_message.
 * @transfer_list: transfers are sequenced through @spi_message.transfers
 *
 * SPI transfers always write the same number of bytes as they read.
 * Protocol drivers should always provide @rx_buf and/or @tx_buf.
 *
 * If the transmit buffer is null, zeroes will be shifted out
 * while filling @rx_buf.  If the receive buffer is null, the data
 * shifted in will be discarded.  Only "len" bytes shift out (or in).
 * It's an error to try to shift out a partial word.  (For example, by
 * shifting out three bytes with word size of sixteen or twenty bits;
 * the former uses two bytes per word, the latter uses four bytes.)
 *
 * In-memory data values are always in native CPU byte order, translated
 * from the wire byte order (big-endian except with SPI_LSB_FIRST).  So
 * for example when bits_per_word is sixteen, buffers are 2N bytes long
 * (@len = 2N) and hold N sixteen bit words in CPU byte order.
 *
 * When the word size of the SPI transfer is not a power-of-two multiple
 * of eight bits, those in-memory words include extra bits.  In-memory
 * words are always seen by protocol drivers as right-justified, so the
 * undefined (rx) or unused (tx) bits are always the most significant bits.
 *
 * All SPI transfers start with the relevant chipselect active.  Normally
 * it stays selected until after the last transfer in a message.  Drivers
 * can affect the chipselect signal using cs_change.
 *
 * (i) If the transfer isn't the last one in the message, this flag is
 * used to make the chipselect briefly go inactive in the middle of the
 * message.  Toggling chipselect in this way may be needed to terminate
 * a chip command, letting a single spi_message perform all of group of
 * chip transactions together.
 *
 * (ii) When the transfer is the last one in the message, the chip may
 * stay selected until the next transfer.  On multi-device SPI busses
 * with nothing blocking messages going to other devices, this is just
 * a performance hint; starting a message to another device deselects
 * this one.  But in other cases, this can be used to ensure correctness.
 * Some devices need protocol transactions to be built from a series of
 * spi_message submissions, where the content of one message is determined
 * by the results of previous messages and where the whole transaction
 * ends when the chipselect goes intactive.
 *
 * The code that submits an spi_message (and its spi_transfers)
 * to the lower layers is responsible for managing its memory.
 * Zero-initialize every field you don't set up explicitly, to
 * insulate against future API updates.  After you submit a message
 * and its transfers, ignore them until its completion callback.
 */
struct spi_transfer {
	/* it's ok if tx_buf == rx_buf (right?)
	 * for MicroWire, one buffer must be null
	 * buffers must work with dma_*map_single() calls, unless
	 *   spi_message.is_dma_mapped reports a pre-existing mapping
	 */
	const void	*tx_buf;
	void		*rx_buf;
	unsigned	len;

	unsigned	cs_change:1;
	u8		bits_per_word;
	u16		delay_usecs;
	u32		speed_hz;

	struct list_head transfer_list;
};

/**
 * struct spi_message - one multi-segment SPI transaction
 * @transfers: list of transfer segments in this transaction
 * @spi: SPI device to which the transaction is queued
 * @actual_length: the total number of bytes that were transferred in all
 *	successful segments
 * @status: zero for success, else negative errno
 * @queue: for use by whichever driver currently owns the message
 * @state: for use by whichever driver currently owns the message
 *
 * A @spi_message is used to execute an atomic sequence of data transfers,
 * each represented by a struct spi_transfer.  The sequence is "atomic"
 * in the sense that no other spi_message may use that SPI bus until that
 * sequence completes.  On some systems, many such sequences can execute as
 * as single programmed DMA transfer.  On all systems, these messages are
 * queued, and might complete after transactions to other devices.  Messages
 * sent to a given spi_device are alway executed in FIFO order.
 *
 * The code that submits an spi_message (and its spi_transfers)
 * to the lower layers is responsible for managing its memory.
 * Zero-initialize every field you don't set up explicitly, to
 * insulate against future API updates.  After you submit a message
 * and its transfers, ignore them until its completion callback.
 */
struct spi_message {
	struct list_head	transfers;

	struct spi_device	*spi;

	/* REVISIT:  we might want a flag affecting the behavior of the
	 * last transfer ... allowing things like "read 16 bit length L"
	 * immediately followed by "read L bytes".  Basically imposing
	 * a specific message scheduling algorithm.
	 *
	 * Some controller drivers (message-at-a-time queue processing)
	 * could provide that as their default scheduling algorithm.  But
	 * others (with multi-message pipelines) could need a flag to
	 * tell them about such special cases.
	 */

	unsigned		actual_length;
	int			status;

	/* for optional use by whatever driver currently owns the
	 * spi_message ...  between calls to spi_async and then later
	 * complete(), that's the spi_master controller driver.
	 */
	struct list_head	queue;
	void			*state;
};

static inline void spi_message_init(struct spi_message *m)
{
	memset(m, 0, sizeof *m);
	INIT_LIST_HEAD(&m->transfers);
}

static inline void
spi_message_add_tail(struct spi_transfer *t, struct spi_message *m)
{
	list_add_tail(&t->transfer_list, &m->transfers);
}

static inline void
spi_transfer_del(struct spi_transfer *t)
{
	list_del(&t->transfer_list);
}

/* All these synchronous SPI transfer routines are utilities layered
 * over the core async transfer primitive.  Here, "synchronous" means
 * they will sleep uninterruptibly until the async transfer completes.
 */

int spi_sync(struct spi_device *spi, struct spi_message *message);

struct spi_device *spi_new_device(struct spi_controller *ctrl,
				  struct spi_board_info *chip);
int spi_register_controller(struct spi_controller *ctrl);

#ifdef CONFIG_SPI
int spi_register_board_info(struct spi_board_info const *info, int num);
#else
static inline int spi_register_board_info(struct spi_board_info const *info,
		int num)
{
	return 0;
}
#endif

/**
 * spi_write - SPI synchronous write
 * @spi: device to which data will be written
 * @buf: data buffer
 * @len: data buffer size
 * Context: can sleep
 *
 * This writes the buffer and returns zero or a negative error code.
 * Callable only from contexts that can sleep.
 */
static inline int
spi_write(struct spi_device *spi, const void *buf, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

/**
 * spi_read - SPI synchronous read
 * @spi: device from which data will be read
 * @buf: data buffer
 * @len: data buffer size
 * Context: can sleep
 *
 * This reads the buffer and returns zero or a negative error code.
 * Callable only from contexts that can sleep.
 */
static inline int
spi_read(struct spi_device *spi, void *buf, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

/* this copies txbuf and rxbuf data; for small transfers only! */
extern int spi_write_then_read(struct spi_device *spi,
		const void *txbuf, unsigned n_tx,
		void *rxbuf, unsigned n_rx);

/**
 * spi_w8r8 - SPI synchronous 8 bit write followed by 8 bit read
 * @spi: device with which data will be exchanged
 * @cmd: command to be written before data is read back
 * Context: can sleep
 *
 * This returns the (unsigned) eight bit number returned by the
 * device, or else a negative error code.  Callable only from
 * contexts that can sleep.
 */
static inline ssize_t spi_w8r8(struct spi_device *spi, u8 cmd)
{
	ssize_t			status;
	u8			result;

	status = spi_write_then_read(spi, &cmd, 1, &result, 1);

	/* return negative errno or unsigned value */
	return (status < 0) ? status : result;
}

extern struct bus_type spi_bus;

struct spi_controller *spi_get_controller(int bus);

static inline int spi_driver_register(struct driver_d *drv)
{
	drv->bus = &spi_bus;
	return register_driver(drv);
}

#define device_spi_driver(drv)	\
	register_driver_macro(device,spi,drv)

#endif /* __INCLUDE_SPI_H */

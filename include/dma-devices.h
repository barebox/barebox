/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __DMA_DEVICES_H
#define __DMA_DEVICES_H

/**
 * enum dma_transfer_direction - dma transfer mode and direction indicator
 * @DMA_MEM_TO_MEM: Async/Memcpy mode
 * @DMA_MEM_TO_DEV: Slave mode & From Memory to Device
 * @DMA_DEV_TO_MEM: Slave mode & From Device to Memory
 * @DMA_DEV_TO_DEV: Slave mode & From Device to Device
 */
enum dma_transfer_direction {
	DMA_MEM_TO_MEM,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
	DMA_DEV_TO_DEV,
	DMA_TRANS_NONE,
};

struct dma_device;

struct dma {
	struct device *dev;
	/*
	 * Written by of_xlate. We assume a single id is enough for now. In the
	 * future, we might add more fields here.
	 */
	unsigned long id;

	struct dma_device *dmad;
};

struct of_phandle_args;

/*
 * struct dma_ops - Driver model DMA operations
 *
 * The uclass interface is implemented by all DMA devices which use
 * driver model.
 */
struct dma_ops {
	/**
	 * of_xlate - Translate a client's device-tree (OF) DMA specifier.
	 *
	 * The DMA core calls this function as the first step in implementing
	 * a client's dma_get_by_*() call.
	 *
	 * If this function pointer is set to NULL, the DMA core will use a
	 * default implementation, which assumes #dma-cells = <1>, and that
	 * the DT cell contains a simple integer DMA Channel.
	 *
	 * At present, the DMA API solely supports device-tree. If this
	 * changes, other xxx_xlate() functions may be added to support those
	 * other mechanisms.
	 *
	 * @dma: The dma struct to hold the translation result.
	 * @args:	The dma specifier values from device tree.
	 * @return 0 if OK, or a negative error code.
	 */
	int (*of_xlate)(struct dma *dma,
			struct of_phandle_args *args);
	/**
	 * request - Request a translated DMA.
	 *
	 * The DMA core calls this function as the second step in
	 * implementing a client's dma_get_by_*() call, following a successful
	 * xxx_xlate() call, or as the only step in implementing a client's
	 * dma_request() call.
	 *
	 * @dma: The DMA struct to request; this has been filled in by
	 *   a previoux xxx_xlate() function call, or by the caller of
	 *   dma_request().
	 * @return 0 if OK, or a negative error code.
	 */
	int (*request)(struct dma *dma);
	/**
	 * rfree - Free a previously requested dma.
	 *
	 * This is the implementation of the client dma_free() API.
	 *
	 * @dma: The DMA to free.
	 * @return 0 if OK, or a negative error code.
	 */
	int (*rfree)(struct dma *dma);
	/**
	 * enable() - Enable a DMA Channel.
	 *
	 * @dma: The DMA Channel to manipulate.
	 * @return zero on success, or -ve error code.
	 */
	int (*enable)(struct dma *dma);
	/**
	 * disable() - Disable a DMA Channel.
	 *
	 * @dma: The DMA Channel to manipulate.
	 * @return zero on success, or -ve error code.
	 */
	int (*disable)(struct dma *dma);
	/**
	 * prepare_rcv_buf() - Prepare/Add receive DMA buffer.
	 *
	 * @dma: The DMA Channel to manipulate.
	 * @dst: The receive buffer pointer.
	 * @size: The receive buffer size
	 * @return zero on success, or -ve error code.
	 */
	int (*prepare_rcv_buf)(struct dma *dma, dma_addr_t dst, size_t size);
	/**
	 * receive() - Receive a DMA transfer.
	 *
	 * @dma: The DMA Channel to manipulate.
	 * @dst: The destination pointer.
	 * @metadata: DMA driver's specific data
	 * @return zero on success, or -ve error code.
	 */
	int (*receive)(struct dma *dma, dma_addr_t *dst, void *metadata);
	/**
	 * send() - Send a DMA transfer.
	 *
	 * @dma: The DMA Channel to manipulate.
	 * @src: The source pointer.
	 * @len: Length of the data to be sent (number of bytes).
	 * @metadata: DMA driver's specific data
	 * @return zero on success, or -ve error code.
	 */
	int (*send)(struct dma *dma, dma_addr_t src, size_t len, void *metadata);
	/**
	 * get_cfg() - Get DMA channel configuration for client's use
	 *
	 * @dma:    The DMA Channel to manipulate
	 * @cfg_id: DMA provider specific ID to identify what
	 *          configuration data client needs
	 * @data:   Pointer to store pointer to DMA driver specific
	 *          configuration data for the given cfg_id (output param)
	 * @return zero on success, or -ve error code.
	 */
	int (*get_cfg)(struct dma *dma, u32 cfg_id, void **data);
	/**
	 * transfer() - Issue a DMA transfer. The implementation must
	 *   wait until the transfer is done.
	 *
	 * @dev: The DMA device
	 * @direction: direction of data transfer (should be one from
	 *   enum dma_direction)
	 * @dst: The destination pointer.
	 * @src: The source pointer.
	 * @len: Length of the data to be copied (number of bytes).
	 * @return zero on success, or -ve error code.
	 */
	int (*transfer)(struct device *dev, int direction, dma_addr_t dst,
			dma_addr_t src, size_t len);
};

struct dma_device {
	struct device *dev;
	const struct dma_ops *ops;
	struct list_head list;
};

int dma_device_register(struct dma_device *dmad);

struct dma *dma_get_by_index(struct device *dev, int index);
struct dma *dma_get_by_name(struct device *dev, const char *name);
int dma_prepare_rcv_buf(struct dma *dma, dma_addr_t dst, size_t size);
int dma_enable(struct dma *dma);
int dma_disable(struct dma *dma);
int dma_get_cfg(struct dma *dma, u32 cfg_id, void **cfg_data);
int dma_send(struct dma *dma, dma_addr_t src, size_t len, void *metadata);
int dma_receive(struct dma *dma, dma_addr_t *dst, void *metadata);
int dma_release(struct dma *dma);

#endif /* __DMA_DEVICES_H */

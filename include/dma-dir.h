#ifndef __DMA_DIR_H
#define __DMA_DIR_H

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

#endif /* __DMA_DIR_H */

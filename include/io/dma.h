#ifndef ROBERTO_DMA_H
#define ROBERTO_DMA_H

#include <stm32f4/stm32f4xx.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DMA_CTRL_STREAMS 8

enum dma_direction {
	DMA_PERIPHERAL_TO_MEM = 0,
	DMA_MEM_TO_PERIPHERAL = 1,
	DMA_MEM_TO_MEM = 2,
};

enum dma_data_size { DMA_BYTE = 0, DMA_HALFWORD = 1, DMA_WORD = 2 };

struct dma_stream {
	const struct dma_controller *ctrl;
	const uint8_t index : 3;
	const uint8_t channel : 3;
};

struct dma_controller {
	const uint32_t enr;
	DMA_TypeDef *regs;
	DMA_Stream_TypeDef *streams[DMA_CTRL_STREAMS];
	unsigned int num;
};

struct dma_config {
	struct dma_stream stream;
	const enum dma_direction direction;
	const enum dma_data_size msize;
	const enum dma_data_size psize;
	const bool minc;
	const bool pinc;
	const volatile void *peripheral_reg;
	const uint8_t *buffer;
	const size_t size;
};

const struct dma_controller dma_controller_1;
const struct dma_controller dma_controller_2;

void dma_init(void);
void dma_setup_transfer(const struct dma_config *config);

#endif // ROBERTO_DMA_H

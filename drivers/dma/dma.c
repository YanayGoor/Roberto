#include <stm32f4/stm32f4xx.h>

#include <io/dma.h>

void dma_enable_controller(const struct dma_controller *ctrl) {
	RCC->AHB1ENR |= ctrl->enr;
}

void dma_setup_transfer(const struct dma_controller *ctrl,
						const struct dma_transfer_stream *transfer) {
	dma_enable_controller(ctrl);
	DMA_Stream_TypeDef *stream = ctrl->streams[transfer->stream];
	stream->CR &= ~DMA_SxCR_EN;
	while (stream->CR & DMA_SxCR_EN) {}
	stream->CR |= (transfer->channel & 1) ? DMA_SxCR_CHSEL_0 : 0;
	stream->CR |= (transfer->channel & 2) ? DMA_SxCR_CHSEL_1 : 0;
	stream->CR |= (transfer->channel & 4) ? DMA_SxCR_CHSEL_2 : 0;
	stream->CR |= transfer->minc ? DMA_SxCR_MINC : 0;
	stream->CR |= transfer->pinc ? DMA_SxCR_PINC : 0;
	stream->CR |= (transfer->direction & 1) ? DMA_SxCR_DIR_0 : 0;
	stream->CR |= (transfer->direction & 2) ? DMA_SxCR_DIR_1 : 0;
	stream->CR |= (transfer->msize & 1) ? DMA_SxCR_MSIZE_0 : 0;
	stream->CR |= (transfer->msize & 2) ? DMA_SxCR_MSIZE_1 : 0;
	stream->CR |= (transfer->psize & 1) ? DMA_SxCR_PSIZE_0 : 0;
	stream->CR |= (transfer->psize & 2) ? DMA_SxCR_PSIZE_1 : 0;
	stream->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;
	stream->M0AR = (uint32_t)transfer->buffer;
	stream->NDTR = transfer->size;
	stream->PAR = (uint32_t)transfer->peripheral_reg;
	stream->FCR = 0;
}

void dma_enable_transfer(const struct dma_controller *ctrl,
						 const struct dma_transfer_stream *transfer) {
	dma_enable_controller(ctrl);
	ctrl->streams[transfer->stream]->CR |= DMA_SxCR_EN;
}

// clang-format off
#define DMA_DEFINE_CONTROLLER(index)                          \
	const struct dma_controller dma_controller_##index = {	  \
		.enr = RCC_AHB1ENR_DMA##index##EN,                    \
    	.regs = DMA##index,                                   \
    	.streams = {                                          \
			DMA##index##_Stream0,                             \
			DMA##index##_Stream1,                             \
			DMA##index##_Stream2,                             \
			DMA##index##_Stream3,                             \
			DMA##index##_Stream4,                             \
			DMA##index##_Stream5,                             \
			DMA##index##_Stream6,                             \
			DMA##index##_Stream7                              \
		}                                         			  \
	}
// clang-format on

DMA_DEFINE_CONTROLLER(1);
DMA_DEFINE_CONTROLLER(2);

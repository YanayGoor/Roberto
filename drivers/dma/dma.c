#include <stm32f4/stm32f4xx.h>

#include <io/dma.h>
#include <kernel/future.h>
#include <kernel/sched.h>

#define GET_STREAM_HEAD(ctrl, stream) (streams[((ctrl)->num - 1) * 8 + stream])

struct dma_stream_head {
	uint32_t pending;
	waitq_head waiting;
	struct dma_reserved_stream *current;
};

struct dma_reserved_stream {
	const struct dma_controller *ctrl;
	uint8_t index;
	struct future future;
};

struct dma_stream_head *streams[DMA_CHANNELS_PER_CTRL * 2];

void dma_enable_controller(const struct dma_controller *ctrl) {
	RCC->AHB1ENR |= ctrl->enr;
}

void dma_configure_stream(const struct dma_controller *ctrl,
						  uint8_t stream_index,
						  const struct dma_transfer_config *transfer) {
	dma_enable_controller(ctrl);
	DMA_Stream_TypeDef *stream = ctrl->streams[stream_index];
	stream->CR &= ~DMA_SxCR_EN;
	while (stream->CR & DMA_SxCR_EN) {}
	stream->CR = 0;
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

struct dma_reserved_stream *
dma_reserve_stream(const struct dma_controller *ctrl, uint8_t index) {
	struct dma_stream_head *head = GET_STREAM_HEAD(ctrl, index);
	head->pending++;
	if (head->pending > 1) { awaitq(&head->waiting); }

	struct dma_reserved_stream *reserved_stream =
		malloc(sizeof(struct dma_reserved_stream));
	reserved_stream->ctrl = ctrl;
	reserved_stream->index = index;
	head->current = NULL;
	return reserved_stream;
}

void dma_release_stream(struct dma_reserved_stream *stream) {
	struct dma_stream_head *head = GET_STREAM_HEAD(stream->ctrl, stream->index);
	head->current = NULL;
	head->pending--;

	wake_up_one(&head->waiting);
}

void dma_setup_transfer(struct dma_reserved_stream *stream,
						struct dma_transfer_config *config) {
	dma_configure_stream(stream->ctrl, stream->index, config);
}

/**
 * start a transfer and block until completed
 */
struct future *dma_start_transfer(struct dma_reserved_stream *stream) {
	stream->ctrl->streams[stream->index]->CR |= DMA_SxCR_EN;
	return &stream->future;
}

void dma_init(void) {
	//	sched_start_task(dma_background_worker, NULL);
	NVIC_EnableIRQ(DMA1_Stream0_IRQn);
	NVIC_EnableIRQ(DMA1_Stream1_IRQn);
	NVIC_EnableIRQ(DMA1_Stream2_IRQn);
	NVIC_EnableIRQ(DMA1_Stream3_IRQn);
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);
	NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	NVIC_EnableIRQ(DMA1_Stream7_IRQn);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	NVIC_EnableIRQ(DMA2_Stream1_IRQn);
	NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	NVIC_EnableIRQ(DMA2_Stream3_IRQn);
	NVIC_EnableIRQ(DMA2_Stream4_IRQn);
	NVIC_EnableIRQ(DMA2_Stream5_IRQn);
	NVIC_EnableIRQ(DMA2_Stream6_IRQn);
	NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

// clang-format off
#define DMA_DEFINE_IRQ_HANDLER(ctrl_index, index, high)					\
	void DMA##ctrl_index##_Stream##index##_IRQHandler(void) {  \
		dma_controller_##ctrl_index.regs->high##IFCR |= DMA_##high##IFCR_CTCIF##index | DMA_##high##IFCR_CDMEIF##index | DMA_##high##IFCR_CTEIF##index; \
    	wake_up(&GET_STREAM_HEAD(&dma_controller_##ctrl_index, index)->current->future);                                                                           \
	}

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
		},                                                    \
		.num = index                                          \
	}
// clang-format on

DMA_DEFINE_CONTROLLER(1);
DMA_DEFINE_CONTROLLER(2);

DMA_DEFINE_IRQ_HANDLER(1, 0, L);
DMA_DEFINE_IRQ_HANDLER(1, 1, L);
DMA_DEFINE_IRQ_HANDLER(1, 2, L);
DMA_DEFINE_IRQ_HANDLER(1, 3, L);
DMA_DEFINE_IRQ_HANDLER(1, 4, H);
DMA_DEFINE_IRQ_HANDLER(1, 5, H);
DMA_DEFINE_IRQ_HANDLER(1, 6, H);
DMA_DEFINE_IRQ_HANDLER(1, 7, H);
DMA_DEFINE_IRQ_HANDLER(2, 0, L);
DMA_DEFINE_IRQ_HANDLER(2, 1, L);
DMA_DEFINE_IRQ_HANDLER(2, 2, L);
DMA_DEFINE_IRQ_HANDLER(2, 3, L);
DMA_DEFINE_IRQ_HANDLER(2, 4, H);
DMA_DEFINE_IRQ_HANDLER(2, 5, H);
DMA_DEFINE_IRQ_HANDLER(2, 6, H);
DMA_DEFINE_IRQ_HANDLER(2, 7, H);
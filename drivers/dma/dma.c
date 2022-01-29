#include <stm32f4/stm32f4xx.h>

#include <io/dma.h>
#include <kernel/future.h>
#include <kernel/sched.h>
#include <sys/queue.h>

#define GET_STREAM_QUEUE(ctrl, stream)                                         \
	(&transfers[((ctrl)->num - 1) * 8 + stream])

enum transfer_status { transfer_pending, transfer_in_progress, transfer_done };

struct dma_transfer {
	const struct dma_stream *stream;
	enum transfer_status status;
	struct future future;
	LIST_ENTRY(dma_transfer) entry;
};

LIST_HEAD(transfers_head, dma_transfer) transfers[8 * 2];

void dma_enable_controller(const struct dma_controller *ctrl) {
	RCC->AHB1ENR |= ctrl->enr;
}

void list_insert_tail(struct transfers_head *head, struct dma_transfer *item) {
	// TODO: Create a tail queue struct for O(1) access.
	if (LIST_EMPTY(head)) {
		LIST_INSERT_HEAD(head, item, entry);
		return;
	}
	struct dma_transfer *last = LIST_FIRST(head);
	while (LIST_NEXT(last, entry) != NULL) {
		last = LIST_NEXT(last, entry);
	}
	LIST_INSERT_AFTER(last, item, entry);
}

void dma_setup_stream_transfer(const struct dma_stream *transfer) {
	dma_enable_controller(transfer->ctrl);
	DMA_Stream_TypeDef *stream = transfer->ctrl->streams[transfer->stream];
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

void dma_enable_transfer(const struct dma_stream *transfer) {
	dma_enable_controller(transfer->ctrl);
	dma_setup_stream_transfer(transfer);
	transfer->ctrl->streams[transfer->stream]->CR |= DMA_SxCR_EN;
}

void dma_setup_transfer(const struct dma_stream *stream) {
	struct dma_transfer *transfer = malloc(sizeof(struct dma_transfer));
	transfer->stream = stream;
	transfer->status = transfer_pending;
	transfer->future = FUTURE_INITIALIZER(&transfer->future);
	list_insert_tail(GET_STREAM_QUEUE(stream->ctrl, stream->stream), transfer);
	await(&transfer->future);
}

void __attribute__((noreturn)) dma_background_worker() {
	while (1) {
		sched_yield();
		for (int i = 0; i < 8 * 2; i++) {
			struct dma_transfer *transfer = LIST_FIRST(&transfers[i]);
			if (transfer == NULL) { continue; }
			if (transfer->status == transfer_pending) {
				transfer->status = transfer_in_progress;
				dma_enable_transfer(transfer->stream);
			}
			if (transfer->status == transfer_done) {
				LIST_REMOVE(transfer, entry);
				wake_up(&transfer->future);
				free(transfer);
			}
		}
	}
}

void dma_init(void) {
	sched_start_task(dma_background_worker, NULL);
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
//		DMA##ctrl_index##_Stream##index->CR &= ~DMA_SxCR_TCIE;  	\

// clang-format off
#define DMA_DEFINE_IRQ_HANDLER(ctrl_index, index, high)					\
	void DMA##ctrl_index##_Stream##index##_IRQHandler(void) {  \
		dma_controller_##ctrl_index.regs->high##IFCR |= DMA_##high##IFCR_CTCIF##index | DMA_##high##IFCR_CDMEIF##index | DMA_##high##IFCR_CTEIF##index;  	\
		LIST_FIRST(                                                	\
			GET_STREAM_QUEUE(&dma_controller_##ctrl_index, index)   \
		)->status = transfer_done;									\
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
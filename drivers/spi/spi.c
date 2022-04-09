#include <io/dma.h>
#include <io/spi.h>
#include <kernel/future.h>
#include <stddef.h>
#include <stdint.h>
#include "kernel/time.h"

struct dma_stream {
	const struct dma_controller *ctrl;
	uint8_t index;
	uint8_t channel;
};

#define SPI_DEFINE_STREAM(controller, stream, channel_index)                   \
	{ .ctrl = &controller, .index = stream, .channel = channel_index }

struct dma_stream spi1_tx_stream_1 = SPI_DEFINE_STREAM(dma_controller_2, 3, 3);
struct dma_stream spi1_tx_stream_2 = SPI_DEFINE_STREAM(dma_controller_2, 5, 3);
struct dma_stream spi1_rx_stream_1 = SPI_DEFINE_STREAM(dma_controller_2, 0, 3);
struct dma_stream spi1_rx_stream_2 = SPI_DEFINE_STREAM(dma_controller_2, 2, 3);
struct dma_stream spi2_tx_stream = SPI_DEFINE_STREAM(dma_controller_1, 4, 0);
struct dma_stream spi2_rx_stream = SPI_DEFINE_STREAM(dma_controller_1, 3, 0);
struct dma_stream spi3_tx_stream_1 = SPI_DEFINE_STREAM(dma_controller_1, 5, 0);
struct dma_stream spi3_tx_stream_2 = SPI_DEFINE_STREAM(dma_controller_1, 7, 0);
struct dma_stream spi3_rx_stream_1 = SPI_DEFINE_STREAM(dma_controller_1, 0, 0);
struct dma_stream spi3_rx_stream_2 = SPI_DEFINE_STREAM(dma_controller_1, 2, 0);

static inline struct dma_stream *
get_tx_stream(const struct spi_module *module) {
	if (module == &spi_module_1) { return &spi1_tx_stream_1; }
	if (module == &spi_module_2) { return &spi2_tx_stream; }
	if (module == &spi_module_3) { return &spi3_tx_stream_1; }
	// TODO: fallback to panic instead
	return NULL;
}

static inline struct dma_stream *
get_rx_stream(const struct spi_module *module) {
	if (module == &spi_module_1) { return &spi1_rx_stream_1; }
	if (module == &spi_module_2) { return &spi2_rx_stream; }
	if (module == &spi_module_3) { return &spi3_rx_stream_1; }
	// TODO: fallback to panic instead
	return NULL;
}

// enum spi_transfer_type { SPI_BYTE, SPI_BUFF };
//
// struct spi_transfer {
//	enum spi_transfer_type type;
//	uint8_t send;
//	uint8_t *recv;
//	uint8_t *send_buff;
//	uint32_t slen;
//	uint8_t *recv_buff;
//	uint32_t rlen;
//};

int setup_send_transfer(const struct spi_module *module,
						 struct dma_reserved_stream *send_stream,
						 uint8_t channel, struct spi_transfer *transfer) {
	if (transfer->type == SPI_BUFF && transfer->send_buff == NULL) { return 0; }
	struct dma_transfer_config send_config = {
		.buffer =
			transfer->type == SPI_BYTE ? &transfer->send : transfer->send_buff,
		.size = transfer->type == SPI_BYTE ? 1 : transfer->slen,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = true,
		.pinc = false,
		.peripheral_reg = (void *)&module->regs->DR,
		.channel = channel,
	};
	dma_setup_transfer(send_stream, &send_config);
	return 1;
}
int setup_recv_transfer(const struct spi_module *module,
						 struct dma_reserved_stream *recv_stream,
						 uint8_t channel, struct spi_transfer *transfer) {
	if ((transfer->type == SPI_BUFF && transfer->recv_buff == NULL) || transfer->recv == 0) { return 0; }
	struct dma_transfer_config recv_config = {
		.buffer =
			transfer->type == SPI_BYTE ? transfer->recv : transfer->recv_buff,
		.size = transfer->type == SPI_BYTE ? 1 : transfer->rlen,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = true,
		.pinc = false,
		.peripheral_reg = (void *)&module->regs->DR,
		.channel = channel,
	};
	dma_setup_transfer(recv_stream, &recv_config);
	return 1;
}

void spi_transmit(const struct spi_module *module,
				  struct spi_transfer *transfers, uint8_t len) {
	struct dma_stream *tx_stream = get_tx_stream(module);
	struct dma_stream *rx_stream = get_rx_stream(module);

	struct dma_reserved_stream *send_stream =
		dma_reserve_stream(tx_stream->ctrl, tx_stream->index);
	struct dma_reserved_stream *recv_stream =
		dma_reserve_stream(rx_stream->ctrl, rx_stream->index);
	struct future *send_future = NULL;
	struct future *recv_future = NULL;
	int send_set_up = setup_send_transfer(&spi_module_1, send_stream, tx_stream->channel,
						&transfers[0]);
	for (int i = 0; i < len; i++) {
		int recv_set_up = setup_recv_transfer(&spi_module_1, send_stream, rx_stream->channel,
							&transfers[i]);
		if (send_set_up) {
			send_future = dma_start_transfer(send_stream);
		}
		if (recv_set_up) {
			recv_future = dma_start_transfer(recv_stream);
		}
		if (send_set_up) { await(send_future); }
		// since the recv ends after the send ends, use that time to setup the
		// next send.
		if (i + 1 < len) {
			send_set_up = setup_send_transfer(&spi_module_1, send_stream, tx_stream->channel,
								&transfers[i + 1]);
		}
		if (recv_set_up) {
			await(recv_future);
		}
	}
	dma_release_stream(send_stream);
	dma_release_stream(recv_stream);
}

// void test_api() {
//	uint8_t recved;
//
//	// read reg
//	spi_transmit((struct spi_transfer[]){{
//											 .type = SPI_BYTE,
//											 .send = 0xaa,
//										 },
//										 {
//											 .type = SPI_BYTE,
//											 .recv = &recved,
//										 }},
//				 2);
//
//	// read buff
//	uint8_t recved2[100];
//	spi_transmit((struct spi_transfer[]){{
//											 .type = SPI_BYTE,
//											 .send = 0xaa,
//										 },
//										 {
//											 .type = SPI_BUFF,
//											 .recv_buff = recved2,
//											 .rlen = 100,
//										 }},
//				 2);
//}
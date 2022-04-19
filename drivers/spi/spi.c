#include <io/dma.h>
#include <io/spi.h>
#include <kernel/future.h>
#include <stddef.h>
#include <stdint.h>

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

int setup_dummy_send_transfer(const struct spi_module *module,
							  struct dma_reserved_stream *send_stream,
							  uint8_t channel, const uint8_t *zero_buff,
							  uint32_t len) {

	struct dma_transfer_config send_config = {
		.buffer = zero_buff,
		.size = len,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = false,
		.pinc = false,
		.peripheral_reg = (void *)&module->regs->DR,
		.channel = channel,
	};
	dma_setup_transfer(send_stream, &send_config);
	return 1;
}
int setup_dummy_recv_transfer(const struct spi_module *module,
							  struct dma_reserved_stream *recv_stream,
							  uint8_t channel, const uint8_t *zero_buff,
							  uint32_t len) {

	struct dma_transfer_config send_config = {
		.buffer = zero_buff,
		.size = len,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_PERIPHERAL_TO_MEM,
		.minc = false,
		.pinc = false,
		.peripheral_reg = (void *)&module->regs->DR,
		.channel = channel,
	};
	dma_setup_transfer(recv_stream, &send_config);
	return 1;
}

int setup_recv_transfer(const struct spi_module *module,
						struct dma_reserved_stream *recv_stream,
						uint8_t channel, struct spi_transfer *transfer) {
	if (transfer->type == SPI_BUFF && transfer->recv_buff == NULL) { return 0; }
	if (transfer->type == SPI_BYTE && transfer->recv == 0) { return 0; }
	struct dma_transfer_config recv_config = {
		.buffer =
			transfer->type == SPI_BYTE ? transfer->recv : transfer->recv_buff,
		.size = transfer->type == SPI_BYTE ? 1 : transfer->rlen,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_PERIPHERAL_TO_MEM,
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
	uint8_t zero_buff = 0;
	uint8_t recv_buff = 0;

	struct dma_reserved_stream *send_stream =
		dma_reserve_stream(tx_stream->ctrl, tx_stream->index);
	struct dma_reserved_stream *recv_stream =
		dma_reserve_stream(rx_stream->ctrl, rx_stream->index);
	struct future *send_future = NULL;
	struct future *recv_future = NULL;
	int send_set_up = setup_send_transfer(module, send_stream,
										  tx_stream->channel, &transfers[0]);

	module->regs->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;

	for (int i = 0; i < len; i++) {
		int recv_set_up = setup_recv_transfer(
			module, recv_stream, rx_stream->channel, &transfers[i]);
		if (recv_set_up && !send_set_up) {
			send_set_up = setup_dummy_send_transfer(
				module, send_stream, tx_stream->channel, &zero_buff,
				transfers[i].type == SPI_BUFF ? transfers[i].rlen : 1);
		}
		if (!recv_set_up && send_set_up) {
			recv_set_up = setup_dummy_recv_transfer(
				module, recv_stream, rx_stream->channel, &recv_buff,
				transfers[i].type == SPI_BUFF ? transfers[i].slen : 1);
		}
		if (recv_set_up) { recv_future = dma_start_transfer(recv_stream); }
		if (send_set_up) { send_future = dma_start_transfer(send_stream); }
		if (send_set_up) { await(send_future); }
		// since the recv ends after the send ends, use that time to setup the
		// next send.
		if (i + 1 < len) {
			send_set_up = setup_send_transfer(
				module, send_stream, tx_stream->channel, &transfers[i + 1]);
		}
		if (recv_set_up) { await(recv_future); }
	}

	module->regs->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

	dma_release_stream(send_stream);
	dma_release_stream(recv_stream);
}

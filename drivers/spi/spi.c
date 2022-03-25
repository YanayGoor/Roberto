#include <io/dma.h>
#include <io/spi.h>
#include <kernel/future.h>
#include <stdint.h>

enum spi_transfer_type { SPI_BYTE, SPI_BUFF };

struct spi_transfer {
	enum spi_transfer_type type;
	uint8_t send;
	uint8_t *recv;
	uint8_t *send_buff;
	uint32_t slen;
	uint8_t *recv_buff;
	uint32_t rlen;
};

void setup_send_transfer(const struct spi_module *module,
						 struct dma_reserved_stream *send_stream,
						 struct spi_transfer *transfer) {
	if (transfer->type == SPI_BUFF && transfer->send_buff == NULL) { return; }
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
		// TODO: choose correct channel + ctrl
		.channel = 0,
	};
	dma_setup_transfer(send_stream, &send_config);
}
void setup_recv_transfer(const struct spi_module *module,
						 struct dma_reserved_stream *recv_stream,
						 struct spi_transfer *transfer) {
	if (transfer->type == SPI_BUFF && transfer->recv_buff == NULL) { return; }
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
		// TODO: choose correct channel + ctrl
		.channel = 0,
	};
	dma_setup_transfer(recv_stream, &recv_config);
}

void spi_transmit(struct spi_transfer *transfers, uint8_t len) {
	struct dma_reserved_stream *send_stream =
		dma_reserve_stream(&dma_controller_1, 3);
	struct dma_reserved_stream *recv_stream =
		dma_reserve_stream(&dma_controller_1, 3);
	struct future *send_future;
	struct future *recv_future;
	setup_send_transfer(&spi_module_1, send_stream, &transfers[0]);
	for (int i = 0; i < len; i++) {
		setup_recv_transfer(&spi_module_1, send_stream, &transfers[i]);
		send_future = dma_start_transfer(send_stream);
		recv_future = dma_start_transfer(recv_stream);
		await(send_future);
		// since the recv ends after the send ends, use that time to setup the
		// next send.
		if (i + 1 < len) {
			setup_send_transfer(&spi_module_1, send_stream, &transfers[i + 1]);
		}
		await(recv_future);
	}
	dma_release_stream(send_stream);
	dma_release_stream(recv_stream);
}

void test_api() {
	uint8_t recved;

	// read reg
	spi_transmit((struct spi_transfer[]){{
											 .type = SPI_BYTE,
											 .send = 0xaa,
										 },
										 {
											 .type = SPI_BYTE,
											 .recv = &recved,
										 }},
				 2);

	// read buff
	uint8_t recved2[100];
	spi_transmit((struct spi_transfer[]){{
											 .type = SPI_BYTE,
											 .send = 0xaa,
										 },
										 {
											 .type = SPI_BUFF,
											 .recv_buff = recved2,
											 .rlen = 100,
										 }},
				 2);
}
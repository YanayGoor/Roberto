#include <io/dma.h>
#include <io/spi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SPI_DEFINE_STREAM(controller, stream, channel_index)                   \
	((struct dma_stream){.ctrl = &controller,                                  \
						 .index = stream,                                      \
						 .channel = channel_index})

#define SPI1_TX_STREAM_1 SPI_DEFINE_STREAM(dma_controller_2, 3, 3)
#define SPI1_TX_STREAM_2 SPI_DEFINE_STREAM(dma_controller_2, 5, 3)
#define SPI1_RX_STREAM_1 SPI_DEFINE_STREAM(dma_controller_2, 0, 3)
#define SPI1_RX_STREAM_2 SPI_DEFINE_STREAM(dma_controller_2, 2, 3)

#define SPI2_TX_STREAM SPI_DEFINE_STREAM(dma_controller_1, 4, 0)
#define SPI2_RX_STREAM SPI_DEFINE_STREAM(dma_controller_1, 3, 0)

#define SPI3_TX_STREAM_1 SPI_DEFINE_STREAM(dma_controller_1, 5, 0)
#define SPI3_TX_STREAM_2 SPI_DEFINE_STREAM(dma_controller_1, 7, 0)
#define SPI3_RX_STREAM_1 SPI_DEFINE_STREAM(dma_controller_1, 0, 0)
#define SPI3_RX_STREAM_2 SPI_DEFINE_STREAM(dma_controller_1, 2, 0)

static inline struct dma_stream get_tx_stream(const struct spi_module *module) {
	if (module == &spi_module_1) { return SPI1_TX_STREAM_1; }
	if (module == &spi_module_2) { return SPI2_TX_STREAM; }
	if (module == &spi_module_3) { return SPI3_TX_STREAM_1; }
	// TODO: fallback to panic instead
	return (struct dma_stream){.ctrl = NULL, .index = 0, .channel = 0};
}

void spi_dma_write(const struct spi_module *module, const uint8_t *buffer,
				   size_t size) {
	const struct dma_config transfer = {
		.stream = get_tx_stream(module),
		.buffer = buffer,
		.size = size,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = true,
		.pinc = false,
		.peripheral_reg = &module->regs->DR,
	};

	spi_wait_not_busy(module);

	module->regs->CR2 |= SPI_CR2_TXDMAEN;

	dma_setup_transfer(&transfer);

	spi_wait_not_busy(module);
	spi_read(module);

	module->regs->CR2 &= ~SPI_CR2_TXDMAEN;
}
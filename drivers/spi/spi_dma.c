#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <io/dma.h>
#include <io/spi.h>

void spi_dma_write(const struct spi_module *module, const uint8_t *buffer, size_t size) {
	const struct dma_stream transfer = {
		// TODO: choose correct channel + ctrl
		.ctrl = &dma_controller_1,
		.stream = 4,
		.buffer = buffer,
		.size = size,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = true,
		.pinc = false,
		.peripheral_reg = (void *)&module->regs->DR,
		// TODO: choose correct channel + ctrl
		.channel = 0,
	};

	module->regs->CR2 |= SPI_CR2_TXDMAEN;

	dma_setup_transfer(&transfer);

	spi_wait_not_busy(module);
	spi_wait_read_ready(module);
	spi_read(module);

	module->regs->CR2 &= ~SPI_CR2_TXDMAEN;
}
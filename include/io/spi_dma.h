#ifndef ROBERTO_SPI_DMA_H
#define ROBERTO_SPI_DMA_H

#include <stddef.h>
#include <stdint.h>

void spi_dma_write(const struct spi_module *module, const uint8_t *buffer,
				   size_t size);

#endif // ROBERTO_SPI_DMA_H

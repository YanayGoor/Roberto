#ifndef SPI_H
#define SPI_H

#include "gpio.h"

#include <stdbool.h>
#include <stdint.h>

enum alt_fn {
	AF0,
	AF1,
	AF2,
	AF3,
	AF4,
	AF5,
	AF6,
	AF7,
	AF8,
	AF9,
	AF10,
	AF11,
	AF12,
	AF13,
	AF14,
	AF15
};

enum peripheral_bus { APB1, APB2 };

struct spi_module {
	SPI_TypeDef *regs;
	uint32_t apb1_enr;
	uint32_t apb2_enr;
	uint8_t alt_fn;
};

struct spi_params {
	const struct gpio_port *sclk_port;
	uint8_t sclk_pin;
	const struct gpio_port *miso_port;
	uint8_t miso_pin;
	const struct gpio_port *mosi_port;
	uint8_t mosi_pin;
	bool crc_enable;
	bool lsb_first;
	bool is_long_frame; /* true for 16 bits, false for 8 bits */
	uint8_t baud_rate;	/* index of prescaler (2, 4, 8 ... 256) */
	bool is_master;
	bool clock_polarity; /* true if clk should be high when idle */
	bool clock_phase;	 /* true if the data capture edge is the second clock
							transition */
};

struct spi_slave {
	const struct gpio_port *ss_port;
	uint8_t ss_pin;
	bool active_pull_up; /* true if the slave is active when the line is pulled
							up */
};

const struct spi_module spi_module_1;
const struct spi_module spi_module_2;
const struct spi_module spi_module_3;

void spi_init(const struct spi_module *module, struct spi_params params);

void spi_write(const struct spi_module *module, uint8_t byte);
uint8_t spi_read(const struct spi_module *module);
uint8_t spi_exchange(const struct spi_module *module, uint8_t byte);

void spi_write_buff(const struct spi_module *module, const uint8_t *buff,
					size_t size);
void spi_read_buff(const struct spi_module *module, uint8_t *buff, size_t size);
void spi_exchange_buff(const struct spi_module *module, const uint8_t *transmit,
					   uint8_t *receive, size_t size);

void spi_slave_init(const struct spi_slave *slave);
void spi_slave_select(const struct spi_slave *slave);
void spi_slave_deselect(const struct spi_slave *slave);

#define SPI_SELECT_SLAVE(slave, block)                                         \
	{                                                                          \
		spi_slave_select(slave);                                               \
		block;                                                                 \
		spi_slave_deselect(slave);                                             \
	}

#endif /* SPI_H */

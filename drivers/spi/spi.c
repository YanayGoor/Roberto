#include <stm32f4/stm32f4xx.h>

#include <io/gpio.h>
#include <io/spi.h>
#include <stdint.h>

static void enable_module_clock(const struct spi_module *module) {
	RCC->APB1ENR |= module->apb1_enr;
	RCC->APB2ENR |= module->apb2_enr;
}

void spi_init(const struct spi_module *module, const struct spi_params params) {
	enable_module_clock(module);
	module->regs->CR1 &= ~SPI_CR1_SPE;

	gpio_init_pin(params.sclk_port,
				  (struct gpio_pin){.pin = params.sclk_pin,
									.mode = GPIO_ALTERNATE,
									.alt_fn = module->alt_fn});
	gpio_init_pin(params.mosi_port,
				  (struct gpio_pin){.pin = params.mosi_pin,
									.mode = GPIO_ALTERNATE,
									.alt_fn = module->alt_fn});
	gpio_init_pin(params.miso_port,
				  (struct gpio_pin){.pin = params.miso_pin,
									.mode = GPIO_ALTERNATE,
									.alt_fn = module->alt_fn});
	module->regs->CR1 |= params.crc_enable ? SPI_CR1_CRCEN : 0;
	module->regs->CR1 |= params.lsb_first ? SPI_CR1_LSBFIRST : 0;
	module->regs->CR1 |= params.is_long_frame ? SPI_CR1_DFF : 0;
	module->regs->CR1 |= params.is_master ? SPI_CR1_MSTR : 0;
	module->regs->CR1 |= params.clock_polarity ? SPI_CR1_CPOL : 0;
	module->regs->CR1 |= params.clock_phase ? SPI_CR1_CPHA : 0;
	module->regs->CR1 |= (params.baud_rate & 1) ? SPI_CR1_BR_0 : 0;
	module->regs->CR1 |= (params.baud_rate & 2) ? SPI_CR1_BR_1 : 0;
	module->regs->CR1 |= (params.baud_rate & 4) ? SPI_CR1_BR_2 : 0;
	module->regs->CR1 |= SPI_CR1_SSM;
	module->regs->CR1 |= SPI_CR1_SSI;
	module->regs->CR2 = 0;

	module->regs->CR1 |= SPI_CR1_SPE;
}

void spi_write(const struct spi_module *module, uint8_t data) {
	module->regs->DR = data;
}

bool spi_read_ready(const struct spi_module *module) {
	return module->regs->SR & SPI_SR_RXNE;
}

bool spi_write_ready(const struct spi_module *module) {
	return module->regs->SR & SPI_SR_TXE;
}

bool spi_is_busy(const struct spi_module *module) {
	return module->regs->SR & SPI_SR_BSY;
}

void spi_wait_read_ready(const struct spi_module *module) {
	while (!spi_read_ready(module)) {};
}

void spi_wait_write_ready(const struct spi_module *module) {
	while (!spi_write_ready(module)) {};
}

void spi_wait_not_busy(const struct spi_module *module) {
	while (spi_is_busy(module)) {};
}

uint8_t spi_read(const struct spi_module *module) {
	return module->regs->DR;
}

void spi_slave_select(const struct spi_slave *slave) {
	gpio_write_partial(slave->ss_port, slave->active_pull_up ? -1 : 0,
					   1 << slave->ss_pin);
}

void spi_slave_deselect(const struct spi_slave *slave) {
	gpio_write_partial(slave->ss_port, slave->active_pull_up ? 0 : -1,
					   1 << slave->ss_pin);
}

void spi_slave_init(const struct spi_slave *slave) {
	gpio_init_pin(slave->ss_port,
				  (struct gpio_pin){.pin = slave->ss_pin, .mode = GPIO_OUTPUT});
	spi_slave_deselect(slave);
}

// clang-format off
#define SPI_DEFINE_MODULE(index, peripheral_bus, alternate_fn)                              \
	const struct spi_module spi_module_##index = {	                                        \
		.regs = SPI##index,                                                                 \
		.apb1_enr = peripheral_bus == APB1 ? RCC_##peripheral_bus##ENR_SPI##index##EN : 0,	\
		.apb2_enr = peripheral_bus == APB2 ? RCC_##peripheral_bus##ENR_SPI##index##EN : 0,  \
        .alt_fn = alternate_fn,                                                             \
	}
// clang-format on

SPI_DEFINE_MODULE(1, APB2, AF5);
SPI_DEFINE_MODULE(2, APB1, AF5);
SPI_DEFINE_MODULE(3, APB1, AF6);

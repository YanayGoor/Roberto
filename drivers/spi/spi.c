#include <stm32f4/stm32f4xx.h>

#include <io/gpio.h>
#include <io/spi.h>
#include <stdint.h>

static void spi_write_reg(const struct spi_module *module, uint8_t data) {
	module->regs->DR = data;
}

static uint8_t spi_read_reg(const struct spi_module *module) {
	return module->regs->DR;
}

static bool spi_is_read_ready(const struct spi_module *module) {
	return module->regs->SR & SPI_SR_RXNE;
}

static bool spi_is_write_ready(const struct spi_module *module) {
	return module->regs->SR & SPI_SR_TXE;
}

static void spi_wait_read_ready(const struct spi_module *module) {
	while (!spi_is_read_ready(module)) {};
}

static void spi_wait_write_ready(const struct spi_module *module) {
	while (!spi_is_write_ready(module)) {};
}

static void enable_module_clock(const struct spi_module *module) {
	RCC->APB1ENR |= module->apb1_enr;
	RCC->APB2ENR |= module->apb2_enr;
}

static void seq_queue_next(const struct spi_module *module,
						   uint8_t first_byte) {
	spi_wait_write_ready(module);
	spi_write_reg(module, first_byte);
}

static uint8_t seq_read_previous(const struct spi_module *module) {
	spi_wait_read_ready(module);
	return spi_read_reg(module);
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

void spi_write(const struct spi_module *module, uint8_t byte) {
	spi_exchange(module, byte);
}

uint8_t spi_read(const struct spi_module *module) {
	return spi_exchange(module, 0);
}

uint8_t spi_exchange(const struct spi_module *module, uint8_t byte) {
	spi_wait_write_ready(module);
	spi_write_reg(module, byte);
	spi_wait_read_ready(module);
	return spi_read_reg(module);
}

void spi_write_buff(const struct spi_module *module, const uint8_t *buff,
					size_t size) {
	if (size == 0) { return; }

	seq_queue_next(module, buff[0]);
	for (size_t index = 1; index < size; index++) {
		/* for each byte in the sequence execpt the first or the last, we first
		 * queue the byte for transmission by writing it into the double buffer,
		 * then after the byte starts to get shifted out we can read the
		 * previous value. */
		seq_queue_next(module, buff[index]);
		seq_read_previous(module);
	}
	seq_read_previous(module);
}

void spi_read_buff(const struct spi_module *module, uint8_t *buff,
				   size_t size) {
	if (size == 0) { return; }

	seq_queue_next(module, 0);
	for (size_t index = 1; index < size; index++) {
		/* for each byte in the sequence that is not the first or the last,	we
		 * first queue the byte for transmission by writing it into the	double
		 * buffer, then after the byte starts to get shifted out we can read the
		 * previous value. */
		seq_queue_next(module, 0);
		buff[index - 1] = seq_read_previous(module);
	}
	buff[size - 1] = seq_read_previous(module);
}

void spi_exchange_buff(const struct spi_module *module, const uint8_t *transmit,
					   uint8_t *receive, size_t size) {
	if (size == 0) { return; }

	seq_queue_next(module, transmit[0]);
	for (size_t index = 1; index < size; index++) {
		/* for each byte in the sequence that is not the first or the last,	we
		 * first queue the byte for transmission by writing it into the	double
		 * buffer, then after the byte starts to get shifted out we can read the
		 * previous value. */
		seq_queue_next(module, transmit[index]);
		receive[index - 1] = seq_read_previous(module);
	}
	receive[size - 1] = seq_read_previous(module);
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

#include "internal.h"

#include <io/enc28j60.h>
#include <io/spi.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

#define LOW_BYTE(hword)	 (hword & 0xffff)
#define HIGH_BYTE(hword) (hword >> 8)

#define TO_HIGH_BYTE(hword) (hword << 8)

static void _write_ctrl_reg(struct enc28j60_controller enc, uint8_t address,
							uint8_t value) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, ENC28J60_WCR(address));
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, value);
		spi_wait_write_ready(enc.module);
	})
}

static uint8_t _read_ctrl_reg(struct enc28j60_controller enc, uint8_t address) {
	uint8_t result;
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, ENC28J60_RCR(address));
		spi_wait_read_ready(enc.module);
		result = spi_read(enc.module);
	})
	return result;
}

static void _set_bits_ctrl_reg(struct enc28j60_controller enc, uint8_t address,
							   uint8_t value) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, ENC28J60_BFS(address));
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, value);
		spi_wait_write_ready(enc.module);
	})
}

static void _select_bank(struct enc28j60_controller enc, uint8_t bank) {
	uint8_t econ1 = _read_ctrl_reg(enc, ENC28J60_ECON1.address);
	_write_ctrl_reg(enc, ENC28J60_ECON1.address, econ1 | (bank & 0x3));
}

void enc28j60_begin_buff_read(struct enc28j60_controller enc) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, ENC28J60_RBM());
}

uint8_t enc28j60_buff_read_byte(struct enc28j60_controller enc) {
	spi_wait_read_ready(enc.module);
	return spi_read(enc.module);
}

void enc28j60_buff_read(struct enc28j60_controller enc, uint8_t *buff,
						size_t size) {
	for (size_t i = 0; i < size; i++) {
		buff[i] = enc28j60_buff_read_byte(enc);
	}
}

void enc28j60_begin_buff_write(struct enc28j60_controller enc) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, ENC28J60_WBM());
}

void enc28j60_buff_write_byte(struct enc28j60_controller enc, uint8_t value) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, value);
}

void enc28j60_buff_write(struct enc28j60_controller enc, const uint8_t *buff,
						 size_t size) {
	for (size_t i = 0; i < size; i++) {
		enc28j60_buff_write_byte(enc, buff[i]);
	}
}

void enc28j60_write_ctrl_reg(struct enc28j60_controller enc,
							 struct enc28j60_ctrl_reg reg, uint16_t value) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_write_ctrl_reg(enc, reg.address, LOW_BYTE(value));
	if (reg.wide) { _write_ctrl_reg(enc, reg.address + 1, HIGH_BYTE(value)); }
}

void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller enc,
								struct enc28j60_ctrl_reg reg, uint16_t value) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_set_bits_ctrl_reg(enc, reg.address, LOW_BYTE(value));
	if (reg.wide) {
		_set_bits_ctrl_reg(enc, reg.address + 1, HIGH_BYTE(value));
	}
}

uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller enc,
								struct enc28j60_ctrl_reg reg) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	uint16_t value = _read_ctrl_reg(enc, reg.address);
	if (reg.wide) {
		value |= TO_HIGH_BYTE(_read_ctrl_reg(enc, reg.address + 1));
	}
	return value;
}

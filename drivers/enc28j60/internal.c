#include "internal.h"

#include <io/enc28j60.h>
#include <io/spi.h>
#include <kernel/time.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

#define LOW_BYTE(hword)	 (hword & 0xff)
#define HIGH_BYTE(hword) (hword >> 8)

#define TO_HIGH_BYTE(hword) (hword << 8)

#define PHY_REG_DELAY_NS 10000

static void _write_ctrl_reg(struct enc28j60_controller *enc, uint8_t address,
							uint8_t value) {
	SPI_SELECT_SLAVE(enc->slave, {
		nsleep(210); // Tcsh
		spi_write(enc->module, WCR_OPCODE(address));
		spi_wait_write_ready(enc->module);
		spi_write(enc->module, value);
		spi_wait_not_busy(enc->module);
		spi_read(enc->module);
		nsleep(210); // Tcsh
	})
}

static uint8_t _read_ctrl_reg(struct enc28j60_controller *enc, uint8_t address,
							  bool dummy_byte) {
	uint8_t result;
	SPI_SELECT_SLAVE(enc->slave, {
		nsleep(210); // Tcsh
		spi_write(enc->module, RCR_OPCODE(address));
		spi_wait_write_ready(enc->module);
		spi_write(enc->module, 0);
		spi_wait_read_ready(enc->module);
		spi_read(enc->module);
		if (dummy_byte) {
			spi_wait_write_ready(enc->module);
			spi_write(enc->module, 0);
			spi_wait_read_ready(enc->module);
			spi_read(enc->module);
		}
		spi_wait_read_ready(enc->module);
		result = spi_read(enc->module);
		nsleep(210); // Tcsh
	})
	return result;
}

static void _set_bits_ctrl_reg(struct enc28j60_controller *enc, uint8_t address,
							   uint8_t value) {
	SPI_SELECT_SLAVE(enc->slave, {
		spi_write(enc->module, BFS_OPCODE(address));
		spi_wait_write_ready(enc->module);
		spi_write(enc->module, value);
		spi_wait_not_busy(enc->module);
		spi_read(enc->module);
		nsleep(210); // Tcsh
	})
}

static void _clear_bits_ctrl_reg(struct enc28j60_controller *enc,
								 uint8_t address, uint8_t value) {
	SPI_SELECT_SLAVE(enc->slave, {
		spi_write(enc->module, BFC_OPCODE(address));
		spi_wait_write_ready(enc->module);
		spi_write(enc->module, value);
		spi_wait_not_busy(enc->module);
		spi_read(enc->module);
		nsleep(210); // Tcsh
	})
}

static void _select_bank(struct enc28j60_controller *enc, uint8_t bank) {
	if (enc->selected_bank == bank) { return; }
	enc->selected_bank = bank;
	_set_bits_ctrl_reg(enc, ENC28J60_ECON1.addr, bank & ECON1_BSEL);
	_clear_bits_ctrl_reg(enc, ENC28J60_ECON1.addr, (~bank) & ECON1_BSEL);
}

void enc28j60_begin_buff_read(struct enc28j60_controller *enc) {
	spi_wait_write_ready(enc->module);
	spi_write(enc->module, RBM_OPCODE());
	spi_wait_read_ready(enc->module);
	spi_read(enc->module);
}

uint8_t enc28j60_buff_read_byte(struct enc28j60_controller *enc) {
	spi_wait_write_ready(enc->module);
	spi_write(enc->module, 1);
	spi_wait_read_ready(enc->module);
	return spi_read(enc->module);
}

void enc28j60_buff_read(struct enc28j60_controller *enc, uint8_t *buff,
						size_t size) {
	for (size_t i = 0; i < size; i++) {
		buff[i] = enc28j60_buff_read_byte(enc);
	}
}

void enc28j60_begin_buff_write(struct enc28j60_controller *enc) {
	spi_wait_write_ready(enc->module);
	spi_write(enc->module, WBM_OPCODE());
}

void enc28j60_buff_write_byte(struct enc28j60_controller *enc, uint8_t value) {
	spi_wait_write_ready(enc->module);
	spi_write(enc->module, value);
	// read previous value
	spi_wait_read_ready(enc->module);
	spi_read(enc->module);
}

void enc28j60_finish_buff_write(struct enc28j60_controller *enc) {
	spi_wait_read_ready(enc->module);
	spi_read(enc->module);
}

void enc28j60_buff_write(struct enc28j60_controller *enc, const uint8_t *buff,
						 size_t size) {
	for (size_t i = 0; i < size; i++) {
		enc28j60_buff_write_byte(enc, buff[i]);
	}
}

void enc28j60_write_ctrl_reg(struct enc28j60_controller *enc,
							 struct enc28j60_ctrl_reg reg, uint16_t value) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_write_ctrl_reg(enc, reg.addr, LOW_BYTE(value));
	if (reg.wide) { _write_ctrl_reg(enc, reg.addr + 1, HIGH_BYTE(value)); }
}

void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller *enc,
								struct enc28j60_ctrl_reg reg, uint16_t value) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_set_bits_ctrl_reg(enc, reg.addr, LOW_BYTE(value));
	if (reg.wide) { _set_bits_ctrl_reg(enc, reg.addr + 1, HIGH_BYTE(value)); }
}

void enc28j60_clear_bits_ctrl_reg(struct enc28j60_controller *enc,
								  struct enc28j60_ctrl_reg reg,
								  uint16_t value) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_clear_bits_ctrl_reg(enc, reg.addr, LOW_BYTE(value));
	if (reg.wide) { _clear_bits_ctrl_reg(enc, reg.addr + 1, HIGH_BYTE(value)); }
}

uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller *enc,
								struct enc28j60_ctrl_reg reg) {
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	uint16_t value = _read_ctrl_reg(enc, reg.addr, reg.dummy_byte);
	if (reg.wide) {
		value |=
			TO_HIGH_BYTE(_read_ctrl_reg(enc, reg.addr + 1, reg.dummy_byte));
	}
	return value;
}

uint16_t enc28j60_read_phy_ctrl_reg(struct enc28j60_controller *enc,
									uint8_t addr) {
	enc28j60_write_ctrl_reg(enc, MIREGADR_REG, addr);
	enc28j60_write_ctrl_reg(enc, MICMD_REG, 1);
	nsleep(PHY_REG_DELAY_NS);
	while (enc28j60_read_ctrl_reg(enc, MISTAT_REG) & 1) {}
	enc28j60_write_ctrl_reg(enc, MICMD_REG, 0);
	return enc28j60_read_ctrl_reg(enc, MIRD_REG);
}

void enc28j60_write_phy_ctrl_reg(struct enc28j60_controller *enc, uint8_t addr,
								 uint16_t value) {
	enc28j60_write_ctrl_reg(enc, MIREGADR_REG, addr);
	enc28j60_write_ctrl_reg(enc, MIWR_REG, value);
	nsleep(PHY_REG_DELAY_NS);
	while (enc28j60_read_ctrl_reg(enc, MISTAT_REG) & 1) {}
}
#include "internal.h"

#include <io/enc28j60.h>
#include <io/spi.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

static void _begin_buff_read(struct enc28j60_controller enc) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, RBM());
}

static uint8_t _buff_read_byte(struct enc28j60_controller enc) {
	spi_wait_read_ready(enc.module);
	return spi_read(enc.module);
}

static void _buff_read(struct enc28j60_controller enc, uint8_t *buff,
					   size_t size) {
	for (size_t i = 0; i < size; i++) {
		buff[i] = _buff_read_byte(enc);
	}
}

static void _begin_buff_write(struct enc28j60_controller enc) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, WBM());
}

static void _buff_write_byte(struct enc28j60_controller enc, uint8_t value) {
	spi_wait_write_ready(enc.module);
	spi_write(enc.module, value);
}

static void _buff_write(struct enc28j60_controller enc, const uint8_t *buff,
						size_t size) {
	for (size_t i = 0; i < size; i++) {
		_buff_write_byte(enc, buff[i]);
	}
}

static void _write_ctrl_reg(struct enc28j60_controller enc, uint8_t address,
							uint8_t value) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, WCR(address));
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, value);
	})
}

static uint8_t _read_ctrl_reg(struct enc28j60_controller enc, uint8_t address,
							  bool dummy_byte) {
	uint8_t result;
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, RCR(address));
		if (dummy_byte) {
			spi_wait_read_ready(enc.module);
			spi_read(enc.module);
		}
		spi_wait_read_ready(enc.module);
		result = spi_read(enc.module);
	})
	return result;
}

static void _set_bits_ctrl_reg(struct enc28j60_controller enc, uint8_t address,
							   uint8_t value) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, BFS(address));
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, value);
	})
}

static void _select_bank(struct enc28j60_controller enc, uint8_t bank) {
	uint8_t econ1 = _read_ctrl_reg(enc, ECON1.address, ECON1.dummy_byte);
	_write_ctrl_reg(enc, ECON1.address, econ1 | (bank & 0x3));
}

void enc28j60_init(struct enc28j60_controller enc) {
	spi_slave_init(&enc.slave);
}

void enc28j60_write_ctrl_reg(struct enc28j60_controller enc,
							 struct ctrl_reg reg, uint8_t value) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_write_ctrl_reg(enc, reg.address, value);
}

void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller enc,
								struct ctrl_reg reg, uint8_t value) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_set_bits_ctrl_reg(enc, reg.address, value);
}

uint8_t enc28j60_read_ctrl_reg(struct enc28j60_controller enc,
							   struct ctrl_reg reg) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	return _read_ctrl_reg(enc, reg.address, reg.dummy_byte);
}

int enc28j60_read_packet(struct enc28j60_controller enc, struct pkt_hdr *header,
						 uint8_t *buffer, size_t size) {
	int err = -1;

	spi_slave_select(&enc.slave);
	_begin_buff_read(enc);

	_buff_read(enc, (uint8_t *)&header, sizeof(header));
	if (header->byte_count > size) { goto done; }
	_buff_read(enc, buffer, header->byte_count);

	// mark packet as handled
	enc28j60_write_ctrl_reg(enc, ERXRDPTL, header->next_pkt_ptr);
	enc28j60_write_ctrl_reg(enc, ERXRDPTH,
							header->next_pkt_ptr >> SIZEOF_BITS(uint8_t));
	// decrement packet counter
	enc28j60_set_bits_ctrl_reg(enc, ECON2,
							   ((union econ2){.pktdec = 1}).serialized);
	err = 0;

done:
	spi_slave_deselect(&enc.slave);
	return err;
}

void enc28j60_read_buff_mem(struct enc28j60_controller enc, uint8_t *buffer,
							size_t size) {
	SPI_SELECT_SLAVE(&enc.slave, {
		_begin_buff_read(enc);
		_buff_read(enc, buffer, size);
	})
}

void enc28j60_write_buff_mem(struct enc28j60_controller enc,
							 const uint8_t *buffer, size_t size) {
	SPI_SELECT_SLAVE(&enc.slave, {
		_begin_buff_write(enc);
		_buff_write(enc, buffer, size);
	})
}

void enc28j60_reset(struct enc28j60_controller enc) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, BRC());
	})
}

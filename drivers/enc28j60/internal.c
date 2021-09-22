#include "internal.h"

#include <io/enc28j60.h>
#include <io/spi.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

#define REG_VALUE(reg, ...) (((union reg){__VA_ARGS__}).serialized)

#define LOW_BYTE(hword)	 (hword & 0xffff)
#define HIGH_BYTE(hword) (hword >> 8)

#define TO_HIGH_BYTE(hword) (hword << 8)

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
							 struct ctrl_reg reg, uint16_t value) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_write_ctrl_reg(enc, reg.address, LOW_BYTE(value));
	if (reg.wide) { _write_ctrl_reg(enc, reg.address + 1, HIGH_BYTE(value)); }
}

void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller enc,
								struct ctrl_reg reg, uint16_t value) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	_set_bits_ctrl_reg(enc, reg.address, LOW_BYTE(value));
	if (reg.wide) {
		_set_bits_ctrl_reg(enc, reg.address + 1, HIGH_BYTE(value));
	}
}

uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller enc,
								struct ctrl_reg reg) {
	// TODO: return to previous bank?
	if (!reg.shared) { _select_bank(enc, reg.bank); }
	uint16_t value = _read_ctrl_reg(enc, reg.address, reg.dummy_byte);
	if (reg.wide) {
		value |=
			TO_HIGH_BYTE(_read_ctrl_reg(enc, reg.address + 1, reg.dummy_byte));
	}
	return value;
}

int enc28j60_receive_packet(struct enc28j60_controller enc, uint16_t address,
							struct pkt_rx_hdr *header, uint8_t *buffer,
							size_t size) {
	int err = -1;

	enc28j60_write_ctrl_reg(enc, ETXST, address);

	spi_slave_select(&enc.slave);
	_begin_buff_read(enc);

	_buff_read(enc, (uint8_t *)header, sizeof(header));
	if (header->byte_count > size) { goto done; }
	_buff_read(enc, buffer, header->byte_count);

	enc28j60_write_ctrl_reg(enc, ERXRDPT, header->next_pkt_ptr);
	enc28j60_set_bits_ctrl_reg(enc, ECON2, REG_VALUE(econ2, .pktdec = 1));
	err = 0;

done:
	spi_slave_deselect(&enc.slave);
	return err;
}

void enc28j60_transmit_packet(struct enc28j60_controller enc, uint16_t address,
							  uint8_t *buffer, size_t size) {
	enc28j60_write_ctrl_reg(enc, ETXST, address);

	SPI_SELECT_SLAVE(&enc.slave, {
		_begin_buff_write(enc);

		_buff_write_byte(enc, REG_VALUE(pkt_tx_hdr, .poverride = 0));
		_buff_write(enc, buffer, size);

		enc28j60_write_ctrl_reg(enc, ETXND, address + size);

		enc28j60_set_bits_ctrl_reg(enc, ECON1, REG_VALUE(econ1, .txrts = 1));
	})
}

void enc28j60_packet_transmit_status(struct enc28j60_controller enc,
									 uint16_t address,
									 union pkt_tx_hdr *header) {
	enc28j60_write_ctrl_reg(enc, ETXST, address);

	SPI_SELECT_SLAVE(&enc.slave, {
		_begin_buff_read(enc);
		_buff_read(enc, (uint8_t *)header, sizeof(header));
	})
}

void enc28j60_reset(struct enc28j60_controller enc) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, SRC());
	})
}

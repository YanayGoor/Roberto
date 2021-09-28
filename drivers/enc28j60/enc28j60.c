#include "internal.h"

#include <io/enc28j60.h>

#define REG_VALUE(reg, ...) (((union enc28j60_##reg){__VA_ARGS__}).serialized)

static enc28j60_buff_addr_t _get_rx_size(const struct enc28j60_params *params) {
	return ENC28J60_LAST_ADDR / (params->rx_weight + params->tx_weight) *
		   params->rx_weight;
}

void enc28j60_init(struct enc28j60_controller enc) {
	enc28j60_buff_addr_t rx_size = _get_rx_size(&enc.params);

	spi_slave_init(&enc.slave);

	// initialize receive buffer
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXST, ENC28J60_LAST_ADDR - rx_size);
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXND, ENC28J60_LAST_ADDR);

	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXRDPT,
							ENC28J60_LAST_ADDR - rx_size);

	// initialize MAC
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_MACON1,
							   REG_VALUE(macon1, .marxen = 1,
										 .rxpaus = enc.params.full_duplex,
										 .txpaus = enc.params.full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MACON3,
							REG_VALUE(macon3, .padcfg = 0, .txcrcen = 0,
									  .frmlnen = 0,
									  .fuldpx = enc.params.full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MACON4,
							REG_VALUE(macon4, .defer = 1));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MAMXFL, enc.params.max_frame_length);
	enc28j60_write_ctrl_reg(enc, ENC28J60_MABBIPG,
							ENC28J60_BBIPG_DEFAULT(enc.params.full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MAIPG,
							ENC28J60_IPG_DEFAULT(enc.params.full_duplex));

	// the documentation does not specify why we need to configure the MAC
	// address. since this is currently only a switch, I will leave the mac
	// address at 0.
}

void enc28j60_reset(struct enc28j60_controller enc) {
	SPI_SELECT_SLAVE(&enc.slave, {
		spi_wait_write_ready(enc.module);
		spi_write(enc.module, ENC28J60_SRC());
		spi_wait_write_ready(enc.module);
	})
}

int enc28j60_receive_packet(struct enc28j60_controller enc,
							enc28j60_buff_addr_t address,
							struct enc28j60_pkt_rx_hdr *header, uint8_t *buffer,
							size_t size) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERDPT, address);

	SPI_SELECT_SLAVE(&enc.slave, {
		enc28j60_begin_buff_read(enc);

		enc28j60_buff_read(enc, (uint8_t *)header, sizeof(header));
		if (header->byte_count > size) {
			// TODO: figure out a better way to handle errors without losing the
			//  nice context manager syntax.
			spi_slave_deselect(&enc.slave);
			return -1;
		}
		enc28j60_buff_read(enc, buffer, header->byte_count);
	})

	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXRDPT, header->next_pkt_ptr);
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON2,
							   REG_VALUE(econ2, .pktdec = 1));
	return 0;
}

void enc28j60_transmit_packet(struct enc28j60_controller enc,
							  enc28j60_buff_addr_t address, uint8_t *buffer,
							  size_t size) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_ETXST, address);
	enc28j60_write_ctrl_reg(enc, ENC28J60_EWRPT, address);

	SPI_SELECT_SLAVE(&enc.slave, {
		enc28j60_begin_buff_write(enc);

		enc28j60_buff_write_byte(enc, REG_VALUE(pkt_tx_hdr, .poverride = 0));
		enc28j60_buff_write(enc, buffer, size);
	})

	enc28j60_write_ctrl_reg(enc, ENC28J60_ETXND, address + size);
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON1,
							   REG_VALUE(econ1, .txrts = 1));
}

void enc28j60_packet_transmit_status(struct enc28j60_controller enc,
									 enc28j60_buff_addr_t address,
									 union enc28j60_pkt_tx_hdr *header) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_EWRPT, address);

	SPI_SELECT_SLAVE(&enc.slave, {
		enc28j60_begin_buff_read(enc);
		enc28j60_buff_read(enc, (uint8_t *)header, sizeof(header));
	})
}

#include "internal.h"

#include <io/enc28j60.h>

#define REG_VALUE(reg, ...) (((union enc28j60_##reg){__VA_ARGS__}).serialized)

#define WAIT_10_NS()                                                           \
	for (int i = 0; i < 10; i++) {}
#define WAIT_50_NS()                                                           \
	for (int i = 0; i < 50; i++) {}

#define WRAP_RX_BUFF(addr, rx_buff_start)                                      \
	(addr < rx_buff_start ? addr + ENC28J60_LAST_ADDR - rx_buff_start : addr)

static enc28j60_buff_addr_t get_rx_buff_size(uint8_t rx_weight,
											 uint8_t tx_weight) {
	return ENC28J60_LAST_ADDR / (rx_weight + tx_weight) * rx_weight;
}

void enc28j60_init(struct enc28j60_controller *enc,
				   const struct spi_module *module,
				   const struct spi_slave *slave, bool full_duplex,
				   uint16_t max_frame_length, uint8_t rx_weight,
				   uint8_t tx_weight) {
	enc28j60_buff_addr_t rx_buff_size = get_rx_buff_size(rx_weight, tx_weight);

	enc->module = module;
	enc->slave = slave;
	enc->full_duplex = full_duplex;
	enc->max_frame_length = max_frame_length;
	enc->rx_buff_start = ENC28J60_LAST_ADDR - rx_buff_size;
	enc->next_pkt_addr = enc->rx_buff_start;
	enc->selected_bank = -1;

	// initialize spi slave
	spi_slave_init(slave);

	// initialize receive buffer
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXST, enc->rx_buff_start);
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXND, ENC28J60_LAST_ADDR);
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXRDPT, ENC28J60_LAST_ADDR);

	enc28j60_write_ctrl_reg(enc, ENC28J60_ERXFCON, 0);

	// initialize MAC
	enc28j60_write_ctrl_reg(enc, ENC28J60_MACON1,
							REG_VALUE(macon1, .marxen = 1,
									  .rxpaus = full_duplex,
									  .txpaus = full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MACON3,
							REG_VALUE(macon3, .padcfg = 0, .txcrcen = 0,
									  .frmlnen = 0, .fuldpx = full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MACON4,
							REG_VALUE(macon4, .defer = 1));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MAMXFL, max_frame_length);
	enc28j60_write_ctrl_reg(enc, ENC28J60_MABBIPG,
							ENC28J60_BBIPG_DEFAULT(full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_MAIPG,
							ENC28J60_IPG_DEFAULT(full_duplex));

	enc28j60_write_phy_ctrl_reg(enc, 0,
								REG_VALUE(phcon1, .pdpxmd = full_duplex));
	enc28j60_write_ctrl_reg(enc, ENC28J60_ECON1, REG_VALUE(econ1, .rxen = 1));
}

void enc28j60_reset(struct enc28j60_controller *enc) {
	SPI_SELECT_SLAVE(enc->slave, {
		WAIT_50_NS() // Tcss
		spi_write(enc->module, ENC28J60_SRC());
		spi_wait_read_ready(enc->module);
		spi_read(enc->module);
		WAIT_10_NS() // Tcsh
	})
	WAIT_50_NS() // Tcsd
}

int enc28j60_receive_packet(struct enc28j60_controller *enc,
							struct enc28j60_pkt_rx_hdr *header, uint8_t *buffer,
							size_t size) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERDPT, enc->next_pkt_addr);

	SPI_SELECT_SLAVE(enc->slave, {
		enc28j60_begin_buff_read(enc);

		enc28j60_buff_read(enc, (uint8_t *)header, sizeof(*header));
		if (header->byte_count > size) {
			// TODO: figure out a better way to handle errors without losing the
			//  nice context manager syntax.
			spi_slave_deselect(enc->slave);
			return -1;
		}
		enc28j60_buff_read(enc, buffer, header->byte_count);
	})

	enc28j60_write_ctrl_reg(
		enc, ENC28J60_ERXRDPT,
		WRAP_RX_BUFF(enc->next_pkt_addr - 1, enc->rx_buff_start));
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON2,
							   REG_VALUE(econ2, .pktdec = 1));
	enc28j60_clear_bits_ctrl_reg(enc, ENC28J60_EIR, 1);

	enc->next_pkt_addr = header->next_pkt_addr;
	return header->byte_count;
}

void enc28j60_transmit_packet(struct enc28j60_controller *enc,
							  enc28j60_buff_addr_t address,
							  const uint8_t *buffer, size_t size) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_ETXST, address);
	enc28j60_write_ctrl_reg(enc, ENC28J60_EWRPT, address);

	SPI_SELECT_SLAVE(enc->slave, {
		enc28j60_begin_buff_write(enc);

		// TODO: revert flags
		enc28j60_buff_write_byte(
			enc, REG_VALUE(pkt_tx_hdr, .poverride = 1, .pcrcen = 1));
		enc28j60_buff_write(enc, buffer, size);

		enc28j60_finish_buff_write(enc);
	})

	enc28j60_write_ctrl_reg(enc, ENC28J60_ETXND, address + size);
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON1,
							   REG_VALUE(econ1, .txrts = 1));
}

void enc28j60_packet_transmit_status(struct enc28j60_controller *enc,
									 enc28j60_buff_addr_t address,
									 union enc28j60_pkt_tx_status *status) {
	enc28j60_write_ctrl_reg(enc, ENC28J60_ERDPT, address);

	SPI_SELECT_SLAVE(enc->slave, {
		enc28j60_begin_buff_read(enc);
		enc28j60_buff_read(enc, (uint8_t *)status, sizeof(*status));
	})
}

uint16_t enc28j60_packets_received(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, ENC28J60_EPKTCNT);
}

bool enc28j60_check_error(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, ENC28J60_EIR) & 1;
}

bool enc28j60_check_tx_busy(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, ENC28J60_ECON1) & (1 << 3);
}

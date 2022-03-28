#include "internal.h"
#include "io/dma.h"
#include "kernel/future.h"
#include "kernel/time.h"

#include <io/enc28j60.h>
#include <memory.h>

#define WAIT_10_NS()                                                           \
	for (int i = 0; i < 10; i++) {}
#define WAIT_50_NS()                                                           \
	for (int i = 0; i < 50; i++) {}

#define EIR_ERRORS (EIR_RXERIF | EIR_TXERIF)

static inline buff_addr_t get_rx_buff_size(uint8_t rx_weight,
										   uint8_t tx_weight) {
	return ENC28J60_LAST_ADDR / (rx_weight + tx_weight) * rx_weight;
}

static inline buff_addr_t wrap_rx_buff_addr(struct enc28j60_controller *enc,
											buff_addr_t addr) {
	if (addr < enc->rx_buff_start) {
		return addr - enc->rx_buff_start + ENC28J60_LAST_ADDR;
	}
	return addr;
}

void enc28j60_init(struct enc28j60_controller *enc,
				   const struct spi_module *module,
				   const struct spi_slave *slave, bool full_duplex,
				   uint16_t max_frame_length, uint8_t rx_weight,
				   uint8_t tx_weight) {
	buff_addr_t rx_buff_size = get_rx_buff_size(rx_weight, tx_weight);

	enc->module = module;
	enc->slave = slave;
	enc->full_duplex = full_duplex;
	enc->max_frame_length = max_frame_length;
	enc->rx_buff_start = ENC28J60_LAST_ADDR - rx_buff_size;

	enc->next_pkt_addr = enc->rx_buff_start;
	enc->selected_bank = -1;
	enc->pkt_tx_status_addr = 0;

	// initialize spi slave
	spi_slave_init(slave);

	// initialize receive buffer
	enc28j60_write_ctrl_reg(enc, ERXST_REG, enc->rx_buff_start);
	enc28j60_write_ctrl_reg(enc, ERXND_REG, ENC28J60_LAST_ADDR);
	enc28j60_write_ctrl_reg(enc, ERXRDPT_REG, ENC28J60_LAST_ADDR);

	enc28j60_write_ctrl_reg(enc, ERXFCON_REG, 0);

	// initialize MAC
	if (full_duplex) {
		enc28j60_write_ctrl_reg(enc, MACON1_REG,
								MACON1_MARXEN | MACON1_RXPAUS | MACON1_TXPAUS);
		enc28j60_write_ctrl_reg(enc, MACON3_REG, MACON3_FULDPX);
	} else {
		enc28j60_write_ctrl_reg(enc, MACON1_REG, MACON1_MARXEN);
		enc28j60_write_ctrl_reg(enc, MACON3_REG, 0);
	}
	enc28j60_write_ctrl_reg(enc, MACON4_REG, MACON4_DEFER);
	enc28j60_write_ctrl_reg(enc, MAMXFL_REG, max_frame_length);
	enc28j60_write_ctrl_reg(enc, MABBIPG_REG,
							ENC28J60_BBIPG_DEFAULT(full_duplex));
	enc28j60_write_ctrl_reg(enc, MAIPG_REG, ENC28J60_IPG_DEFAULT(full_duplex));

	enc28j60_write_phy_ctrl_reg(enc, 0, full_duplex ? PHCON1_PDPXMD : 0);
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON1, ECON1_RXEN);
}

void enc28j60_reset(struct enc28j60_controller *enc) {
	SPI_SELECT_SLAVE(enc->slave, {
		WAIT_50_NS() // Tcss
		spi_write(enc->module, SRC_OPCODE());
		spi_wait_read_ready(enc->module);
		spi_read(enc->module);
		WAIT_10_NS() // Tcsh
	})
	WAIT_50_NS() // Tcsd
}

int enc28j60_receive_packet(struct enc28j60_controller *enc,
							struct enc28j60_pkt_rx_hdr *header, uint8_t *buffer,
							size_t size) {
	enc28j60_write_ctrl_reg(enc, ERDPT_REG, enc->next_pkt_addr);

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

	enc28j60_write_ctrl_reg(enc, ERXRDPT_REG,
							wrap_rx_buff_addr(enc, enc->next_pkt_addr - 1));
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON2, ECON2_PKTDEC);
	enc28j60_clear_bits_ctrl_reg(enc, ENC28J60_EIR, EIR_RXERIF);

	enc->next_pkt_addr = header->next_pkt_addr;
	return header->byte_count;
}

void enc28j60_transmit_packet(struct enc28j60_controller *enc,
							  const uint8_t *buffer, size_t size,
							  uint8_t flags) {
	enc28j60_write_ctrl_reg(enc, ETXST_REG, 0);
	enc28j60_write_ctrl_reg(enc, EWRPT_REG, 0);

	struct dma_reserved_stream *stream =
		dma_reserve_stream(&dma_controller_1, 4);

	unsigned char *new_buff = malloc(size + 2);
	memcpy(new_buff + 2, buffer, size);
	new_buff[0] = WBM_OPCODE();
	new_buff[1] = ENC28J60_POVERRIDE | flags;

	const struct dma_transfer_config transfer = {
		// TODO: choose correct channel + ctrl
		//		.ctrl = &dma_controller_1,
		//		.stream = 4,
		.buffer = new_buff,
		.size = size + 2,
		.psize = DMA_BYTE,
		.msize = DMA_BYTE,
		.direction = DMA_MEM_TO_PERIPHERAL,
		.minc = true,
		.pinc = false,
		.peripheral_reg = (void *)&enc->module->regs->DR,
		// TODO: choose correct channel + ctrl
		.channel = 0,
	};

	enc->module->regs->CR2 |= SPI_CR2_TXDMAEN;

	dma_setup_transfer(stream, &transfer);

	SPI_SELECT_SLAVE(enc->slave, {
		nsleep(50); // Tcsh
		await(dma_start_transfer(stream));
		nsleep(210); // Tcsh
	})

	spi_wait_not_busy(enc->module);
	spi_wait_read_ready(enc->module);
	spi_read(enc->module);

	enc->module->regs->CR2 &= ~SPI_CR2_TXDMAEN;

	free(new_buff);
	dma_release_stream(stream);

	//	SPI_SELECT_SLAVE(enc->slave, {
	//		enc28j60_begin_buff_write(enc);
	//
	//		if (flags) {
	//			enc28j60_buff_write_byte(enc, ENC28J60_POVERRIDE | flags);
	//		} else {
	//			enc28j60_buff_write_byte(enc, 0);
	//		}
	//		enc28j60_buff_write(enc, buffer, size);
	//
	//		enc28j60_finish_buff_write(enc);
	//	})

	enc28j60_write_ctrl_reg(enc, ETXND_REG, size);
	enc28j60_set_bits_ctrl_reg(enc, ENC28J60_ECON1, ECON1_TXRTS);

	enc->pkt_tx_status_addr = size + 1;

	//	spi_read(enc->module);
}

void enc28j60_last_transmitted_pkt_status(
	struct enc28j60_controller *enc, struct enc28j60_pkt_tx_status *status) {
	enc28j60_write_ctrl_reg(enc, ERDPT_REG, enc->pkt_tx_status_addr);

	SPI_SELECT_SLAVE(enc->slave, {
		enc28j60_begin_buff_read(enc);
		enc28j60_buff_read(enc, (uint8_t *)status, sizeof(*status));
	})
}

uint16_t enc28j60_packets_received(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, EPKTCNT_REG);
}

bool enc28j60_get_errors(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, ENC28J60_EIR) & EIR_ERRORS;
}

bool enc28j60_get_tx_busy(struct enc28j60_controller *enc) {
	return enc28j60_read_ctrl_reg(enc, ENC28J60_ECON1) & ECON1_TXRTS;
}

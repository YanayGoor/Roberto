#ifndef ENC28J60_H
#define ENC28J60_H

#include <io/spi.h>

#define MAC_ADDR_BYTES 6

struct enc28j60_controller {
	const struct spi_module *module;
	const struct spi_slave *slave;
	/* cached state */
	int8_t selected_bank;
	uint16_t next_pkt_addr;
	/* parameters */
	bool full_duplex;
	uint16_t max_frame_length;
	uint16_t rx_queue_size;
};

struct enc28j60_pkt_rx_hdr {
	uint16_t next_pkt_addr;
	uint16_t byte_count;
	union {
		struct {
			uint8_t long_drop_event : 1;
			uint8_t reserved1 : 1;
			uint8_t carrier_event : 1;
			uint8_t reserved2 : 1;
			uint8_t crc_err : 1;
			uint8_t len_check_err : 1;
			uint8_t len_out_of_range : 1;
			uint8_t received_ok : 1;
			uint8_t multicast : 1;
			uint8_t broadcast : 1;
			uint8_t dribble_nibble : 1;
			uint8_t ctrl_frame : 1;
			uint8_t pause_ctrl_frame : 1;
			uint8_t unknown_ctrl_frame : 1;
			uint8_t vlan : 1;
			uint8_t zero : 1;
		};
		uint16_t flags;
	};
};

union enc28j60_pkt_tx_hdr {
	struct {
		uint8_t poverride : 1;
		uint8_t pcrcen : 1;
		uint8_t ppaden : 1;
		uint8_t phugeen : 1;
	};
	uint8_t serialized;
};

void enc28j60_init(struct enc28j60_controller *enc,
				   const struct spi_module *module,
				   const struct spi_slave *slave, bool full_duplex,
				   uint16_t max_frame_length, uint8_t rx_weight,
				   uint8_t tx_weight);
void enc28j60_reset(struct enc28j60_controller *enc);
int enc28j60_receive_packet(struct enc28j60_controller *enc,
							struct enc28j60_pkt_rx_hdr *header, uint8_t *buffer,
							size_t size);
void enc28j60_transmit_packet(struct enc28j60_controller *enc, uint16_t address,
							  uint8_t *buffer, size_t size);
void enc28j60_packet_transmit_status(struct enc28j60_controller *enc,
									 uint16_t address,
									 union enc28j60_pkt_tx_hdr *header);

#endif // ENC28J60_H

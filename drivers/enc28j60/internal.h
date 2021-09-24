#ifndef ENC28J60_INTERNAL_H
#define ENC28J60_INTERNAL_H

#include <assert.h>
#include <io/enc28j60.h>
#include <stdbool.h>
#include <stdint.h>

#define ENC28J60_LAST_ADDR ((buff_addr_t)0x1fff)

#define BBIPG_DEFAULT(full_duplex) (full_duplex ? 0x15 : 0x12)
#define IPG_DEFAULT(full_duplex)   (full_duplex ? 0x12 : 0x0c12)

typedef uint16_t buff_addr_t;

union opcode {
	struct {
		uint8_t opcode : 3;
		uint8_t argument : 5;
	};
	uint8_t serialized;
};

struct ctrl_reg {
	bool shared; /* true is register can be accessed from all banks */
	uint8_t bank : 2;
	uint8_t address : 5;
	bool dummy_byte; /* true if dummy byte is returned first when reading the
						register */
	bool wide;		 /* true if value is split to low and high registers */
};

struct pkt_rx_hdr {
	uint16_t next_pkt_ptr;
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

union pkt_tx_hdr {
	struct {
		uint8_t poverride : 1;
		uint8_t pcrcen : 1;
		uint8_t ppaden : 1;
		uint8_t phugeen : 1;
	};
	uint8_t serialized;
};

union econ2 {
	struct {
		uint8_t reserved1 : 3;
		uint8_t vrps : 1;
		uint8_t reserved2 : 1;
		uint8_t pwrsv : 1;
		uint8_t pktdec : 1;
		uint8_t autoinc : 1;
	};
	uint8_t serialized;
};

union econ1 {
	struct {
		uint8_t bsel : 2;
		uint8_t rxen : 1;
		uint8_t txrts : 1;
		uint8_t csumen : 1;
		uint8_t dmast : 1;
		uint8_t rxrst : 1;
		uint8_t txrst : 1;
	};
	uint8_t serialized;
};

union macon1 {
	struct {
		uint8_t marxen : 1;
		uint8_t passall : 1;
		uint8_t rxpaus : 1;
		uint8_t txpaus : 1;
	};
	uint8_t serialized;
};

union macon3 {
	struct {
		uint8_t fuldpx : 1;
		uint8_t frmlnen : 1;
		uint8_t hfrmen : 1;
		uint8_t phdren : 1;
		uint8_t txcrcen : 1;
		uint8_t padcfg : 3;
	};
	uint8_t serialized;
};

union macon4 {
	struct {
		uint8_t reserved : 4;
		uint8_t nobkoff : 1;
		uint8_t bpen : 1;
		uint8_t defer : 1;
	};
	uint8_t serialized;
};

#define RCR(arg) ((union opcode){.opcode = 0x0, .argument = arg}).serialized
#define RBM()	 ((union opcode){.opcode = 0x1, .argument = 0x1a}).serialized
#define WCR(arg) ((union opcode){.opcode = 0x2, .argument = arg}).serialized
#define WBM()	 ((union opcode){.opcode = 0x3, .argument = 0x1a}).serialized
#define BFS(arg) ((union opcode){.opcode = 0x4, .argument = arg}).serialized
#define BFC(arg) ((union opcode){.opcode = 0x5, .argument = arg}).serialized
#define SRC()	 ((union opcode){.opcode = 0x7, .argument = 0x1f}).serialized

#define ECON1 ((struct ctrl_reg){.shared = true, .address = 0x1f})
#define ECON2 ((struct ctrl_reg){.shared = true, .address = 0x1e})
#define ESTAT ((struct ctrl_reg){.shared = true, .address = 0x1d})
#define EIR	  ((struct ctrl_reg){.shared = true, .address = 0x1c})
#define EIE	  ((struct ctrl_reg){.shared = true, .address = 0x1b})

#define MAMXFL	((struct ctrl_reg){.bank = 2, .address = 0x0a, .wide = true})
#define MAIPG	((struct ctrl_reg){.bank = 2, .address = 0x06, .wide = true})
#define MABBIPG ((struct ctrl_reg){.bank = 2, .address = 0x04})
#define MACON4	((struct ctrl_reg){.bank = 2, .address = 0x03})
#define MACON3	((struct ctrl_reg){.bank = 2, .address = 0x02})
#define MACON1	((struct ctrl_reg){.bank = 2, .address = 0x00})

#define ERXRDPT ((struct ctrl_reg){.bank = 0, .address = 0x0c, .wide = true})
#define ERXND	((struct ctrl_reg){.bank = 0, .address = 0x0a, .wide = true})
#define ERXST	((struct ctrl_reg){.bank = 0, .address = 0x08, .wide = true})
#define ETXND	((struct ctrl_reg){.bank = 0, .address = 0x06, .wide = true})
#define ETXST	((struct ctrl_reg){.bank = 0, .address = 0x04, .wide = true})
#define EWRPT	((struct ctrl_reg){.bank = 0, .address = 0x02, .wide = true})
#define ERDPT	((struct ctrl_reg){.bank = 0, .address = 0x00, .wide = true})

void enc28j60_write_ctrl_reg(struct enc28j60_controller enc,
							 struct ctrl_reg reg, uint16_t value);
void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller enc,
								struct ctrl_reg reg, uint16_t value);
uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller enc,
								struct ctrl_reg reg);
int enc28j60_receive_packet(struct enc28j60_controller enc, uint16_t address,
							struct pkt_rx_hdr *header, uint8_t *buffer,
							size_t size);
void enc28j60_transmit_packet(struct enc28j60_controller enc, uint16_t address,
							  uint8_t *buffer, size_t size);
void enc28j60_packet_transmit_status(struct enc28j60_controller enc,
									 uint16_t address,
									 union pkt_tx_hdr *header);

#endif // ENC28J60_INTERNAL_H

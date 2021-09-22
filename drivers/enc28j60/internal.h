#ifndef ENC28J60_INTERNAL_H
#define ENC28J60_INTERNAL_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

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

#define ERXRDPT ((struct ctrl_reg){.bank = 0, .address = 0x0c, .wide = true})

#define ETXND ((struct ctrl_reg){.bank = 0, .address = 0x06, .wide = true})
#define ETXST ((struct ctrl_reg){.bank = 0, .address = 0x04, .wide = true})
#define EWRPT ((struct ctrl_reg){.bank = 0, .address = 0x2, .wide = true})
#define ERDPT ((struct ctrl_reg){.bank = 0, .address = 0x0, .wide = true})

#endif // ENC28J60_INTERNAL_H

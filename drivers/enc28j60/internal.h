#ifndef ENC28J60_INTERNAL_H
#define ENC28J60_INTERNAL_H

#include <assert.h>
#include <io/enc28j60.h>
#include <stdbool.h>
#include <stdint.h>

#define ENC28J60_LAST_ADDR ((enc28j60_buff_addr_t)0x1fff)

#define ENC28J60_BBIPG_DEFAULT(full_duplex) (full_duplex ? 0x15 : 0x12)
#define ENC28J60_IPG_DEFAULT(full_duplex)	(full_duplex ? 0x12 : 0x0c12)

typedef uint16_t enc28j60_buff_addr_t;

union enc28j60_opcode {
	struct {
		uint8_t opcode : 3;
		uint8_t argument : 5;
	};
	uint8_t serialized;
};

struct enc28j60_ctrl_reg {
	bool shared; /* true is register can be accessed from all banks */
	uint8_t bank : 2;
	uint8_t address : 5;
	bool wide; /* true if value is split to low and high registers */
};

union enc28j60_econ2 {
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

union enc28j60_econ1 {
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

union enc28j60_macon1 {
	struct {
		uint8_t marxen : 1;
		uint8_t passall : 1;
		uint8_t rxpaus : 1;
		uint8_t txpaus : 1;
	};
	uint8_t serialized;
};

union enc28j60_macon3 {
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

union enc28j60_macon4 {
	struct {
		uint8_t reserved : 4;
		uint8_t nobkoff : 1;
		uint8_t bpen : 1;
		uint8_t defer : 1;
	};
	uint8_t serialized;
};

#define ENC28J60_OPCODE(...) (((union enc28j60_opcode){__VA_ARGS__}).serialized)
#define ENC28J60_REG(...)	 ((struct enc28j60_ctrl_reg){__VA_ARGS__})

#define ENC28J60_RCR(arg) ENC28J60_OPCODE(.opcode = 0x0, .argument = arg)
#define ENC28J60_RBM()	  ENC28J60_OPCODE(.opcode = 0x1, .argument = 0x1a)
#define ENC28J60_WCR(arg) ENC28J60_OPCODE(.opcode = 0x2, .argument = arg)
#define ENC28J60_WBM()	  ENC28J60_OPCODE(.opcode = 0x3, .argument = 0x1a)
#define ENC28J60_BFS(arg) ENC28J60_OPCODE(.opcode = 0x4, .argument = arg)
#define ENC28J60_BFC(arg) ENC28J60_OPCODE(.opcode = 0x5, .argument = arg)
#define ENC28J60_SRC()	  ENC28J60_OPCODE(.opcode = 0x7, .argument = 0x1f)

#define ENC28J60_ECON1 ENC28J60_REG(.shared = true, .address = 0x1f)
#define ENC28J60_ECON2 ENC28J60_REG(.shared = true, .address = 0x1e)
#define ENC28J60_ESTAT ENC28J60_REG(.shared = true, .address = 0x1d)
#define ENC28J60_EIR   ENC28J60_REG(.shared = true, .address = 0x1c)
#define ENC28J60_EIE   ENC28J60_REG(.shared = true, .address = 0x1b)

#define ENC28J60_MAMXFL	 ENC28J60_REG(.bank = 2, .address = 0x0a, .wide = true)
#define ENC28J60_MAIPG	 ENC28J60_REG(.bank = 2, .address = 0x06, .wide = true)
#define ENC28J60_MABBIPG ENC28J60_REG(.bank = 2, .address = 0x04)
#define ENC28J60_MACON4	 ENC28J60_REG(.bank = 2, .address = 0x03)
#define ENC28J60_MACON3	 ENC28J60_REG(.bank = 2, .address = 0x02)
#define ENC28J60_MACON1	 ENC28J60_REG(.bank = 2, .address = 0x00)

#define ENC28J60_ERXRDPT ENC28J60_REG(.bank = 0, .address = 0x0c, .wide = true)
#define ENC28J60_ERXND	 ENC28J60_REG(.bank = 0, .address = 0x0a, .wide = true)
#define ENC28J60_ERXST	 ENC28J60_REG(.bank = 0, .address = 0x08, .wide = true)
#define ENC28J60_ETXND	 ENC28J60_REG(.bank = 0, .address = 0x06, .wide = true)
#define ENC28J60_ETXST	 ENC28J60_REG(.bank = 0, .address = 0x04, .wide = true)
#define ENC28J60_EWRPT	 ENC28J60_REG(.bank = 0, .address = 0x02, .wide = true)
#define ENC28J60_ERDPT	 ENC28J60_REG(.bank = 0, .address = 0x00, .wide = true)

void enc28j60_write_ctrl_reg(struct enc28j60_controller enc,
							 struct enc28j60_ctrl_reg reg, uint16_t value);
void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller enc,
								struct enc28j60_ctrl_reg reg, uint16_t value);
uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller enc,
								struct enc28j60_ctrl_reg reg);

void enc28j60_begin_buff_read(struct enc28j60_controller enc);
uint8_t enc28j60_buff_read_byte(struct enc28j60_controller enc);
void enc28j60_buff_read(struct enc28j60_controller enc, uint8_t *buff,
						size_t size);

void enc28j60_begin_buff_write(struct enc28j60_controller enc);
void enc28j60_buff_write_byte(struct enc28j60_controller enc, uint8_t value);
void enc28j60_buff_write(struct enc28j60_controller enc, const uint8_t *buff,
						 size_t size);

#endif // ENC28J60_INTERNAL_H

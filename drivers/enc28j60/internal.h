#ifndef ENC28J60_INTERNAL_H
#define ENC28J60_INTERNAL_H

#include <assert.h>
#include <io/enc28j60.h>
#include <stdbool.h>
#include <stdint.h>

#define ENC28J60_LAST_ADDR ((buff_addr_t)0x1fff)

#define ENC28J60_BBIPG_DEFAULT(full_duplex) (full_duplex ? 0x15 : 0x12)
#define ENC28J60_IPG_DEFAULT(full_duplex)	(full_duplex ? 0x12 : 0x0c12)

typedef uint16_t buff_addr_t;

union enc28j60_opcode {
	struct {
		uint8_t argument : 5;
		uint8_t opcode : 3;
	};
	uint8_t serialized;
};

struct enc28j60_ctrl_reg {
	bool shared; /* true is register can be accessed from all banks */
	uint8_t bank : 2;
	uint8_t addr : 5;
	bool wide;		 /* true if value is split to low and high registers */
	bool dummy_byte; /* true if value is split to low and high registers */
};

#define EIR_RXERIF 1
#define EIR_TXERIF (1 << 1)
#define EIR_TXIF   (1 << 3)
#define EIR_LINKIF (1 << 4)
#define EIR_DMAIF  (1 << 5)
#define EIR_PKTIF  (1 << 6)

#define ECON2_VRPS	  (1 << 3)
#define ECON2_PWRSV	  (1 << 5)
#define ECON2_PKTDEC  (1 << 6)
#define ECON2_AUTOINC (1 << 7)

#define ECON1_BSEL	 0b11
#define ECON1_RXEN	 (1 << 2)
#define ECON1_TXRTS	 (1 << 3)
#define ECON1_CSUMEN (1 << 4)
#define ECON1_DMAST	 (1 << 5)
#define ECON1_RXRST	 (1 << 6)
#define ECON1_TXRST	 (1 << 7)

#define MACON1_MARXEN  1
#define MACON1_PASSALL (1 << 1)
#define MACON1_RXPAUS  (1 << 2)
#define MACON1_TXPAUS  (1 << 3)

#define MACON3_FULDPX  1
#define MACON3_FRMLNEN (1 << 1)
#define MACON3_HFRMEN  (1 << 2)
#define MACON3_PHDREN  (1 << 3)
#define MACON3_TXCRCEN (1 << 4)
#define MACON3_PADCFG  (0b111 << 5)

#define MACON4_NOBKOFF (1 << 4)
#define MACON4_BPEN	   (1 << 5)
#define MACON4_DEFER   (1 << 6)

#define PHCON1_PDPXMD  (1 << 8)
#define PHCON1_PPWRSV  (1 << 11)
#define PHCON1_PLOOPBK (1 << 14)
#define PHCON1_PRST	   (1 << 15)

#define ENC28J60_POVERRIDE 1

#define DEF_OPCODE(...) (((union enc28j60_opcode){__VA_ARGS__}).serialized)
#define DEF_REG(...)	((struct enc28j60_ctrl_reg){__VA_ARGS__})

#define RCR_OPCODE(arg) DEF_OPCODE(.opcode = 0x0, .argument = arg)
#define RBM_OPCODE()	DEF_OPCODE(.opcode = 0x1, .argument = 0x1a)
#define WCR_OPCODE(arg) DEF_OPCODE(.opcode = 0x2, .argument = arg)
#define WBM_OPCODE()	DEF_OPCODE(.opcode = 0x3, .argument = 0x1a)
#define BFS_OPCODE(arg) DEF_OPCODE(.opcode = 0x4, .argument = arg)
#define BFC_OPCODE(arg) DEF_OPCODE(.opcode = 0x5, .argument = arg)
#define SRC_OPCODE()	DEF_OPCODE(.opcode = 0x7, .argument = 0x1f)

#define ENC28J60_ECON1 DEF_REG(.shared = true, .addr = 0x1f)
#define ENC28J60_ECON2 DEF_REG(.shared = true, .addr = 0x1e)
#define ENC28J60_ESTAT DEF_REG(.shared = true, .addr = 0x1d)
#define ENC28J60_EIR   DEF_REG(.shared = true, .addr = 0x1c)
#define ENC28J60_EIE   DEF_REG(.shared = true, .addr = 0x1b)

#define MISTAT_REG DEF_REG(.bank = 3, .addr = 0x0a, .dummy_byte = true)

#define MIRD_REG                                                               \
	DEF_REG(.bank = 2, .addr = 0x18, .wide = true, .dummy_byte = true)
#define MIWR_REG                                                               \
	DEF_REG(.bank = 2, .addr = 0x16, .wide = true, .dummy_byte = true)
#define MIREGADR_REG DEF_REG(.bank = 2, .addr = 0x14, .dummy_byte = true)
#define MICMD_REG	 DEF_REG(.bank = 2, .addr = 0x12, .dummy_byte = true)
#define MAMXFL_REG                                                             \
	DEF_REG(.bank = 2, .addr = 0x0a, .wide = true, .dummy_byte = true)
#define MAIPG_REG                                                              \
	DEF_REG(.bank = 2, .addr = 0x06, .wide = true, .dummy_byte = true)
#define MABBIPG_REG DEF_REG(.bank = 2, .addr = 0x04, .dummy_byte = true)
#define MACON4_REG	DEF_REG(.bank = 2, .addr = 0x03, .dummy_byte = true)
#define MACON3_REG	DEF_REG(.bank = 2, .addr = 0x02, .dummy_byte = true)
#define MACON1_REG	DEF_REG(.bank = 2, .addr = 0x00, .dummy_byte = true)

#define EPKTCNT_REG DEF_REG(.bank = 1, .addr = 0x19)
#define ERXFCON_REG DEF_REG(.bank = 1, .addr = 0x18)

#define ERXWRPT_REG DEF_REG(.bank = 0, .addr = 0x0e, .wide = true)
#define ERXRDPT_REG DEF_REG(.bank = 0, .addr = 0x0c, .wide = true)
#define ERXND_REG	DEF_REG(.bank = 0, .addr = 0x0a, .wide = true)
#define ERXST_REG	DEF_REG(.bank = 0, .addr = 0x08, .wide = true)
#define ETXND_REG	DEF_REG(.bank = 0, .addr = 0x06, .wide = true)
#define ETXST_REG	DEF_REG(.bank = 0, .addr = 0x04, .wide = true)
#define EWRPT_REG	DEF_REG(.bank = 0, .addr = 0x02, .wide = true)
#define ERDPT_REG	DEF_REG(.bank = 0, .addr = 0x00, .wide = true)

void enc28j60_write_ctrl_reg(struct enc28j60_controller *enc,
							 struct enc28j60_ctrl_reg reg, uint16_t value);
void enc28j60_set_bits_ctrl_reg(struct enc28j60_controller *enc,
								struct enc28j60_ctrl_reg reg, uint16_t value);
void enc28j60_clear_bits_ctrl_reg(struct enc28j60_controller *enc,
								  struct enc28j60_ctrl_reg reg, uint16_t value);
uint16_t enc28j60_read_ctrl_reg(struct enc28j60_controller *enc,
								struct enc28j60_ctrl_reg reg);

void enc28j60_begin_buff_read(struct enc28j60_controller *enc);
uint8_t enc28j60_buff_read_byte(struct enc28j60_controller *enc);
void enc28j60_buff_read(struct enc28j60_controller *enc, uint8_t *buff,
						size_t size);

void enc28j60_begin_buff_write(struct enc28j60_controller *enc);
void enc28j60_buff_write_byte(struct enc28j60_controller *enc, uint8_t value);
void enc28j60_buff_write(struct enc28j60_controller *enc, const uint8_t *buff,
						 size_t size);
void enc28j60_finish_buff_write(struct enc28j60_controller *enc);

uint16_t enc28j60_read_phy_ctrl_reg(struct enc28j60_controller *enc,
									uint8_t addr);
void enc28j60_write_phy_ctrl_reg(struct enc28j60_controller *enc, uint8_t addr,
								 uint16_t value);

#endif // ENC28J60_INTERNAL_H

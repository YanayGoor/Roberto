#ifndef ENC28J60_H
#define ENC28J60_H

#include <io/spi.h>

#define MAC_ADDR_BYTES 6

struct enc28j60_params {
	bool full_duplex;
	uint16_t max_frame_length; /* default should be 1518 */
	uint8_t rx_weight;
	uint8_t tx_weight;
	uint8_t mac_addr[MAC_ADDR_BYTES];
};

struct enc28j60_controller {
	const struct spi_module *module;
	const struct spi_slave slave;
	const struct enc28j60_params params;
};

void enc28j60_init(struct enc28j60_controller enc);
void enc28j60_reset(struct enc28j60_controller enc);

#endif // ENC28J60_H

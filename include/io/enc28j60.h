#ifndef ENC28J60_H
#define ENC28J60_H

#include <io/spi.h>

struct enc28j60_controller {
	const struct spi_module *module;
	const struct spi_slave slave;
};

void enc28j60_init(struct enc28j60_controller enc);

#endif // ENC28J60_H

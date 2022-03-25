//#include <stdint.h>
//
// enum spi_transmission_type { SPI_BYTE, SPI_BUFF };
//
// struct spi_transmission {
//	enum spi_transmission_type type;
//	uint8_t send;
//	uint8_t *recv;
//	uint8_t *send_buff;
//	uint32_t slen;
//	uint8_t *recv_buff;
//	uint32_t rlen;
//};
//
// void spi_transmit(struct spi_transmission *transmissions, uint8_t len) {}
//
// void test_api() {
//	uint8_t recved;
//
//	// read reg
//	spi_transmit((struct spi_transmission[]){{
//												 .type = SPI_BYTE,
//												 .send = 0xaa,
//											 },
//											 {
//												 .type = SPI_BYTE,
//												 .recv = &recved,
//											 }},
//				 2);
//
//	// read buff
//	uint8_t recved2[100];
//	spi_transmit((struct spi_transmission[]){{
//												 .type = SPI_BYTE,
//												 .send = 0xaa,
//											 },
//											 {
//												 .type = SPI_BUFF,
//												 .recv_buff = recved2,
//												 .rlen = 100,
//											 }},
//				 2);
//}
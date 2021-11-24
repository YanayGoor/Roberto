# Roberto
Home router implemented from scratch using STM32 microcontroller and ENC28J60 ethernet interface boards in the C language.

## Goals
- Experimenting in bare-metal programming and microcontrollers
- Deep-dive into switching, routing and other networking aspects
- Experimenting with developing OS features but in a more natural way
- Creating and maintaining a larger C codebase that requires more design work

## Roadmap
*Note: The roadmap is user-feature oriented and the OS features will be built througout the process as needed.*

### MVPs
- [ ] Hardware drivers
- [ ] Repeater logic (2 controllers)
- [ ] Switching without MAC table
- [ ] Switching
- [ ] Routing & LAN

### Additional features
*(in no particular order)*
- [ ] DHCP
- [ ] NAT
- [ ] Firewall
- [ ] WLAN
- [ ] Gigabit Ethernet

## Installation
for Ubuntu:
```shell
sudo apt install gcc-arm-none-eabi stlink-tools cmake
```

## TODOs
- [ ] Hardware drivers
  - [ ] Use defines instead of union/structs for enc registers in driver.
  - [ ] Remove addr arg from transmit packet in enc driver.
  - [ ] Return all errors from enc driver and mask them outside.
  - [ ] Rewrite spi write/read to be more high level to ensure read buffer is always cleaned up and support buffers.

- [ ] Repeater logic (2 controllers)
  - [ ] Add another controller to loop.
  - [ ] Add internal packet buffer.
  - [ ] Make enc driver non-blocking using DMA so the 2 controllers can work in parallel.

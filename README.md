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
- [ ] Forwarding frames (1 rx port to 1 tx port)
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

###

## Installation
for Ubuntu:
```shell
sudo apt install gcc-arm-none-eabi stlink-tools cmake
```

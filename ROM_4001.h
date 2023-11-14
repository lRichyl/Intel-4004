#pragma once
#include <cstdint>

const int ROM_MEM = 256; // bytes

// The 4004 is capable of addressing up to 4096 bytes of memory, for this 16 4001 ROM chips are connected to the data bus. 
// To identify every chip, the chip is tagged on fabrication to be a chip number from 0 to 15.
// So when receiving the 12 bit address, the chip with the number equal to the top 4 bits of the adress is the one
// from which the next program instruction is read. 

// In the case of the I/O port, the chip selected is indicated by the SRC instruction.

struct ROM4001
{
	uint8_t io_port; // Unlike the RAM chip, each of the 4 bit signals on this port can be either an input or an output.
	uint8_t cm_port; //Chip enable signal.
	uint8_t memory[ROM_MEM];
};

void read_address_from_bus_to_ROM();
uint8_t read_from_ROM(ROM4001 *roms);


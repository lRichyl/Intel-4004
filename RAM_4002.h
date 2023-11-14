#pragma once
#include <cstdint>

// Characters are 4 bit wide.
const int MEM_CHARACTERS    = 16;
const int STATUS_CHARACTERS = 4;
const int REGS_PER_RAM      = 4;
const int CHIPS_PER_BANK    = 4;

struct MemRegister {
	uint8_t main_characters[MEM_CHARACTERS];
	uint8_t status_characters[STATUS_CHARACTERS];
};


struct RAM4002
{
	MemRegister registers[REGS_PER_RAM];
	uint8_t output_port; 
	uint8_t cm_port; //Chip enable signal.
};

// To select any of the 4 chips on a particular bank the 4002-1 and the 4002-2 were used.
// The chips numbers are set by previously connecting their terminals to Vdd or ground.
// If the chip's 4002-1 P0 port is connected to ground, it's set as the chip 0, if P0 is connected to Vdd it's set as chip 1.
// If the chip's 4002-2 P0 port is connected to ground, it's set as the chip 2, if P0 is connected to Vdd it's set as chip 3.

// In the case of this emulator the chip will be selected utilizing the chip number provided by the SRC instruction directly.
struct RAMBank {
	RAM4002 chips[CHIPS_PER_BANK];
};


#include "ROM_4001.h"
#include "CPU_4004.h"

static uint16_t current_address; // This is shared by all the ROM banks.

void read_address_from_bus_to_ROM(){
	// This will be executed three times during STAGE_A, so the 3 4bit numbers are being shifted in to complete the 12 bit address.
	uint8_t address_fragment = read_bus();
	current_address = (current_address >> 4) | (address_fragment << 8);
	current_address &= ~(0xF << 12); // To prevent any issues we set the 4 MSBs to 0 because they're not used.
}


uint8_t read_from_ROM(ROM4001 *roms){
	static int M_count = 1;
	uint8_t rom_index = current_address >> 8;
	uint8_t page_address = (uint8_t)current_address;
	uint8_t result = roms[rom_index].memory[page_address] >> (4 * M_count);
	result &= ~(0xF0);
	M_count--;
	if(M_count == -1) M_count = 1;

	return result;
}
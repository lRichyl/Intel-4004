#pragma once
#include "RAM_4002.h"
#include "ROM_4001.h"

#include <cstdint>

const int REGISTER_COUNT = 16;
const int STACK_LEVELS   = 3;
const int RAM_BANKS      = 8;
const int ROM_COUNT      = 16;

enum Stage {
	STAGE_A, // During this stage the CPU sends the 12 bits address to the data bus.
	STAGE_M, // During this stage the opcode and the operand are read from the bus after the ROM sends it through the bus.
	STAGE_X, // During this stage the operations indicated by the opcode are performed.
	STAGE_COUNT
};

// The 4004 is a 4bit computer, so even though the following variables are declared as 8bit, 
// they will be handled as 4 bit.
struct CPU4004 {
	uint8_t bus_data;
	uint8_t instruction;
	uint8_t accumulator;
	uint8_t carry_flag;
	uint8_t cm_ram;		// RAM bank selection
	uint8_t cm_rom;		// ROM chip select
	uint8_t reset;		// Must be held high for 64 clock cycles (8 instruction cycles)

	Stage stage;
	uint8_t stages_durations[STAGE_COUNT];
	
	uint8_t registers[REGISTER_COUNT];

	uint8_t sp; //Stack pointer.
	uint16_t pc; // Program counter.
	uint16_t stack[STACK_LEVELS]; // Only 12 bits are used.

	uint32_t rom_size;
	bool wait_for_next_instruction_cycle;
};

void run(const char *ROM_name); // This functions starts the emulation.
// void init_cpu(CPU4004 *cpu);
// void reset(CPU4004 *cpu);
void load_binary_file_to_ROM(CPU4004 *cpu, const char *filename);

uint8_t read_bus();
void write_bus(uint8_t data);
uint8_t read_test_result(uint8_t register_index);

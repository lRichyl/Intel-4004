#include "CPU_4004.h"

#include <chrono>
#include <windows.h>
#include <cassert>


using namespace std::chrono;

const int CLOCK_SPEED       = 740000; //Hz
const float CYCLE_PERIOD    = 1.0f / CLOCK_SPEED;
const float CYCLE_PERIOD_ns = CYCLE_PERIOD * 1e9;

struct InstructionState{
	uint8_t OPR;
	uint8_t OPA;
	uint8_t previous_OPR;
	uint8_t previous_OPA;
	uint8_t opcode;
	bool is_16_bit;
};

static CPU4004 cpu;
static RAMBank rams[RAM_BANKS];
static ROM4001 roms[ROM_COUNT];

uint8_t read_bus(){
	return cpu.bus_data;
}

void write_bus(uint8_t data){
	cpu.bus_data = data;
}

// Test results will always be stored in the last register. The storing of the result has to be done directly in the tested program.
uint8_t read_test_result(uint8_t reg_index){
	uint8_t result = (cpu.registers[reg_index * 2] << 4) | cpu.registers[(reg_index * 2) + 1];
	return result;
}

static void set_stages_durations(){
	// Duration is in clock cycles.
	cpu.stages_durations[0] = 3; // A stage duration.
	cpu.stages_durations[1] = 2; // M stage duration.
	cpu.stages_durations[2] = 3; // X stage duration.
}

// This is used to initialize the CPU the first time aswell.
static void reset(CPU4004 *cpu){
	memset(cpu, 0, sizeof(CPU4004));
	set_stages_durations();
}

static void push_address_to_stack(){
	cpu.stack[cpu.sp] = cpu.pc++;
	cpu.sp++;
	
	if(cpu.sp == STACK_LEVELS) cpu.sp == 0; // If more than 3 addresses are push to the stack it goes around overwriting the old addresses.
}

static uint8_t verify_if_at_end_of_page(){
	// If the instruction is at the end of a memory page, jump to the next page.
	uint8_t rom_index = cpu.pc >> 8;
	uint8_t current_page_location = cpu.pc; // Get the page by truncating the address at 8 bits.
	if(current_page_location == ROM_MEM - 1 && rom_index < ROM_COUNT) rom_index++; 
	return rom_index;
}

static void execute_wide_instruction(InstructionState *instr_state, void(*wide_instruction)(InstructionState instr_state)){
	if(instr_state->opcode == instr_state->OPR){
		instr_state->is_16_bit = true;
	}
	else if(instr_state->opcode == instr_state->previous_OPR){
		wide_instruction(*instr_state);
		
		instr_state->is_16_bit = false;
	}
}

static void decode_instruction(uint8_t instruction){
	static InstructionState instr_state;
	instr_state.OPR = cpu.instruction >> 4;
	instr_state.OPA = cpu.instruction & 0x0F;
	
	if(!instr_state.is_16_bit){
		instr_state.opcode = instr_state.OPR;
	} else{
		instr_state.opcode = instr_state.previous_OPR;
	}
	switch(instr_state.opcode){
		case 0x00:{
			// NOP. No operation.
			break;
		}
		case 0x01: {   // JCN instruction, conditional jump. 2 word instruction.	
			execute_wide_instruction(&instr_state, [] (InstructionState instr_state) {
				uint8_t conditions  = instr_state.previous_OPA;
				uint8_t new_address = (instr_state.OPR << 4) | instr_state.OPA;
				{
					// Execute the instruction.
					uint8_t CN1 = conditions & 0x8;  // Invert the jump condition.
					uint8_t CN2 = conditions & 0x4;  // Jump if the accumulator is 0.
					uint8_t CN3 = conditions & 0x2;  // Jump if the carry is 0.
					uint8_t CN4 = conditions & 0x1;  // Jump if test signal pin is 0.
					
					uint8_t rom_index = verify_if_at_end_of_page();
					
					if(!CN1){ // Do not invert the condition.
						if(cpu.accumulator == 0 || cpu.carry_flag == 1){
							cpu.pc = (rom_index << 8) | new_address;
						}
					}
					else{ // Inverted conditions.
						if(cpu.accumulator != 0 || cpu.carry_flag == 0){
							cpu.pc = (rom_index << 8) | new_address;
						}
					}
					cpu.pc -= 1; // The program counter is decreased by one because it is always incremented by one before the next instruction.
				}
				
			}
			);
			
			break;
		} 
		
		case 0x02: {   // FIM instruction, 2 word instruction. Store the second word directly into the register pair indicated by the first word OPA.
			execute_wide_instruction(&instr_state, [] (InstructionState instr_state) {
				uint8_t register_pair = (instr_state.previous_OPA & 0xE) >> 1;
				uint8_t new_value     = (instr_state.OPR << 4) | instr_state.OPA;
				uint8_t D2            = (new_value & 0xF0) >> 4;
				uint8_t D1            = new_value & 0x0F;
				{
					// Execute the instruction.
					cpu.registers[register_pair * 2]       = D2;
					cpu.registers[(register_pair * 2) + 1] = D1;
				}
			});
			
			break;
		} 
		
		case 0x03: { 
			if(instr_state.OPA & 0x01){  // JIN instruction. Jump indirect. The contents of the designated register pair is loaded into the lower 8 bits of the program counter.
				uint8_t target_rp = instr_state.OPA >> 1;
				uint8_t R0 = cpu.registers[target_rp * 2];       // RRR0.
				uint8_t R1 = cpu.registers[(target_rp * 2) + 1]; // RRR1.
				uint8_t new_address = (R0 << 4) | R1;
				
				uint8_t rom_index = verify_if_at_end_of_page();
				
				cpu.pc = (rom_index << 8) | new_address;
			}
			else{ // FIN instruction. Fetch indirect from ROM. Send index RP 0(register pair) as an address. Fetched data is stored in the RP designated in the OPA.
				uint8_t target_rp = instr_state.OPA >> 1;
				uint8_t R0 = cpu.registers[0];       // First part of the register pair.
				uint8_t R1 = cpu.registers[1];       // Second part of the register pair.
				uint8_t address = (R0 << 4) | R1;
				
				uint8_t rom_index = verify_if_at_end_of_page();
				
				cpu.registers[target_rp * 2]       = roms[rom_index].memory[address] >> 4;
				cpu.registers[(target_rp * 2) + 1] = roms[rom_index].memory[address] & 0x0F;			
			}
			
			break;
		}
		
		case 0x04: { // JUN instruction. Jump unconditionally. Set the program counter to the lower 12 bits of this wide instruction.
			execute_wide_instruction(&instr_state, [] (InstructionState instr_state) {
				uint8_t new_address = (instr_state.previous_OPA << 8) | (instr_state.OPR << 4) | instr_state.OPA;
				cpu.pc = new_address - 1; 
			});
			
			break;
		}
		
		case 0x05: { // JMS Jump to subroutine.  Set the program counter to the lower 12 bits of this wide instruction and store the last address in the stack.
			execute_wide_instruction(&instr_state, [] (InstructionState instr_state) {
				push_address_to_stack();
				uint8_t new_address = (instr_state.previous_OPA << 8) | (instr_state.OPR << 4) | instr_state.OPA;
				cpu.pc = new_address - 1; 
			});
			
			break;
		}
		
		case 0x06:{ // INC instruction. Increment the contents of register RRRR.
			uint8_t reg_index = instr_state.OPA;
			uint8_t new_value = (cpu.registers[reg_index] + 1) & 0x0F;
			cpu.registers[reg_index] = new_value;
			break;
		}
		
		case 0x07:{ // ISZ instruction. Increment the contents of register RRRR and jump to address AAA if the result is 0.
			execute_wide_instruction(&instr_state, [] (InstructionState instr_state) {
				uint8_t reg_index = instr_state.previous_OPA;
				uint8_t new_value = (cpu.registers[reg_index] + 1) & 0x0F;
				if(new_value != 0){
					uint8_t rom_index = verify_if_at_end_of_page();
					uint8_t new_address = (rom_index << 8) | (instr_state.OPR << 4) | (instr_state.OPA);
					cpu.pc = new_address - 1; 
				}
			});
			
			break;
		}
	}
	
	
	
	instr_state.previous_OPR = instr_state.OPR;
	instr_state.previous_OPA = instr_state.OPA;
}

void run(const char *ROM_name){
	reset(&cpu);
	load_binary_file_to_ROM(&cpu, ROM_name);
	while (true) {
		
		static int instruction_cycle_count = 0;
		auto begin = high_resolution_clock::now();

		// One instruction cycle takes 8 clock cycles, of which Stage_A takes 3, Stage_M takes 2 and Stage_X takes 3.
		switch (cpu.stage) {
			case STAGE_A:{
				//uint8_t bus_data = (cpu.pc) >> (4 * instruction_cycle_count);
				write_bus((cpu.pc) >> (4 * instruction_cycle_count));
				 // Read 4 bits of data from the bus data, which in this stage is the program counter.
				 // This operation is done 3 times to read the complete 12 bit address.
				read_address_from_bus_to_ROM();
				if (instruction_cycle_count == 2) cpu.stage = STAGE_M;
				break;
			}
			case STAGE_M:{
				// Here we read the 8 bit instruction from the address sent in the previous step.
				// Every one of these read operations retrieve 4 bits so this operation is done twice.
				uint8_t read = read_from_ROM(roms);
				cpu.instruction = (cpu.instruction << 4) | read;
				if (instruction_cycle_count == 4) cpu.stage = STAGE_X;
				break;
			}
			case STAGE_X:{
				// In this stage the instructions get decoded and executed.
				// enum XTime{
					// X1,
					// X2,
					// X3
				// };
				// static XTime x_time;
				
				if(!cpu.wait_for_next_instruction_cycle){
					decode_instruction(cpu.instruction);
				}
				cpu.wait_for_next_instruction_cycle = true;
				
				// For the time being only the time the 3 X cycles take is simulated, but the instructions are instantly executed.
				// switch (x_time){
					// case X1:{
						
						// x_time = X2;
						// break;
					// }
					
					// case X2:{
						
						// x_time = X3;
						// break;
					// }
					
					// case X3:{
						
						// x_time = X1;
						// break;
					// }
				// }
				
				
				if (instruction_cycle_count == 7){
					cpu.wait_for_next_instruction_cycle = false;
					cpu.stage = STAGE_A;
					cpu.pc++;
				}
				break;
			}
		}
		
		
		instruction_cycle_count++;
		if (instruction_cycle_count == 8) instruction_cycle_count = 0;

		auto end = high_resolution_clock::now();
		auto elapsed = duration_cast<nanoseconds>(end - begin);
		while(elapsed.count() < CYCLE_PERIOD_ns - 50) {
			end = high_resolution_clock::now();
			elapsed = duration_cast<nanoseconds>(end - begin);
		}
		// For the system to be running at 740kHz, the elapsed time should be close to 1350 ns.
		// Because of the precision of these clock, the currently measured time is of 1400 ns, which is close enough
		// for the purpose of emulation.
		// printf("Cycle Time: %d ns\n", (int)elapsed.count());
		
		if(cpu.pc >= cpu.rom_size) return; // Reached end of the ROM.
	}
}



void load_binary_file_to_ROM(CPU4004 *cpu, const char *filename){
	FILE *file;

	if ((fopen_s(&file, filename, "rb")) != 0) {
		printf("The file cannot be read.\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	int current_rom = 0;
	int current_mem_address = 0;
	while(1) {

		if(ftell(file) == size) {
			break;
		}

		uint8_t byte;
		fread(&byte, sizeof(byte), 1, file);
		roms[current_rom].memory[current_mem_address] =  byte;

		current_mem_address++;
		if(current_mem_address == ROM_MEM){
			current_mem_address = 0;
			current_rom++;
		}
		cpu->rom_size = (current_rom * ROM_MEM) + current_mem_address;
		
		//printf("ROM contents: %X\n", byte);
		assert(current_rom <= ROM_COUNT);


	}
	//printf("ROM size: %d\n", cpu->rom_size);
	fclose(file);
}

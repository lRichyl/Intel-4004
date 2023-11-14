#include "CPU_4004.h"


#include <stdio.h>
#include <chrono>
#include <string.h>


using namespace std::chrono;

struct Test{
	const char *filename;
	uint8_t result_register_pair;
	uint8_t expected_result;
};

static Test tests[] = {
	// {"JCN.asm", 0x05}, //// Pending test.
	{"tests/FIM0.asm", 0x0, 0xA1},
	{"tests/FIM1.asm", 0x1, 0xB2},
	{"tests/FIM2.asm", 0x2, 0xC3},
	{"tests/FIM3.asm", 0x3, 0xD4},
	{"tests/FIM4.asm", 0x4, 0xE5},
	{"tests/FIM5.asm", 0x5, 0x1A},
	{"tests/FIM6.asm", 0x6, 0x5D},
	{"tests/FIM7.asm", 0x7, 0xAF},
	{"tests/FIN.asm", 0x1, 0xAA},
	{"tests/FIN_EOP.asm",  0x5, 0x2D},
	{"tests/JIN.asm",  0x5, 0x2D},
	{"tests/JIN_EOP.asm",  0x6, 0xCD},
	{"tests/JUN.asm",  0x4, 0xDD},
	// {"tests/JMS.asm",  0x4, 0xDD}, ////Pending test
	{"tests/INC.asm",  0x7, 0x05},
	{"tests/INC_overflow.asm",  0x2, 0x01},
	{"tests/ISZ.asm",  0x6, 0xEE},
	{"tests/ISZ_zero.asm",  0x6, 0xDD}
};

int main(int argc, char **argv) {
	while(1){
		char rom_name[100];
		printf("The ROM's path has to be relative to the exe.\n");
		printf("Enter the name of the ROM: ");
		scanf_s("%s", rom_name, sizeof(rom_name));
		//printf("%s\n", rom_name);
		printf("\n");
		
		if(!strcmp("test", rom_name)){
			int number_of_tests = sizeof(tests) / sizeof(Test);
			for(int i = 0; i < number_of_tests; i++){
				run(tests[i].filename);
				uint8_t result = read_test_result(tests[i].result_register_pair);
				
				if(result == tests[i].expected_result){
					printf("%s PASSED\n", tests[i].filename);
				}
				else{
					printf("%s FAILED.   Reg value: %X\n", tests[i].filename, result);
				}
			}
		}
		else if(!strcmp("exit", rom_name)){
			return 0;
		}
		else{
			run(rom_name);
			// TODO: Print the instructions.
		}

		
		
		
		printf("\n\n");
	}
	

	return 0;
}
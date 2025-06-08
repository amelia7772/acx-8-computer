#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <iostream>
#include <windows.h>
#include <conio.h>
using namespace std;

unsigned char* reg = new unsigned char[8]; //registers

char* ROM = new char[256];
char* RAM = new char[65536];

char* STACK = new char[256];
unsigned char stack_pointer = 0;

unsigned char ROM_program_counter = 0;
unsigned short RAM_program_counter = 0;

bool ROM_prgram_excution_mode = true;
bool RAM_program_excution_mode = false;

bool is_first_instruction_register_filled = false;
bool is_second_instruction_register_filled = false;

bool should_branch = false;

char* keyboard_buffer = new char[32];
unsigned char keyboard_buffer_pointer = 0;

int counter_for_save = 0;

enum Instruction_type {
	NOP = 0,
	STR,
	MOVI,
	MOV,
	ADD,
	SUB,
	MUL,
	DIV,
	DIVRM,
	AND,
	OR,
	NOT,
	XOR,
	NAND,
	NOR,
	XNOR,
	SHFR,
	SHFL,
	CMP,
	CMPI,
	CMPZ,
	PUSH,
	PULL,
	BEQ,
	BNE,
	BGR,
	BLE,
	BNGR,
	BNLE,
	B,
	LDR,
	IOCALL
};

enum IO_devices_IDs{
	//1d == 00000001b
	TTY_DISPLAY = 1,
	//2d == 00000010b
	RAM_program_excution_mode_ID = 2,
	//3d == 00000011b
	ROM_program_excution_mode_ID = 3,
	//129d == 10000001b
	KEYBOARD = 129
};

void disable_echoing_user_input() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
	cout << "user input echo disabled successfully" << endl;
}

void check_for_keypresses() {
	while (true) {
		char ch;

		//the program pauses here until a key is pressed
		ch = _getch();
		//cout << ch;

		if (keyboard_buffer_pointer <= 32) {
			if (ch == '\r') {
				keyboard_buffer[keyboard_buffer_pointer] = ch;
				keyboard_buffer_pointer++;
				if (keyboard_buffer_pointer <= 32) {
					keyboard_buffer[keyboard_buffer_pointer] = '\n';
					keyboard_buffer_pointer++;
				}
			}
			else if (ch == '\b') {
				keyboard_buffer[keyboard_buffer_pointer] = ch;
				keyboard_buffer_pointer++;
				if (keyboard_buffer_pointer <= 32) {
					keyboard_buffer[keyboard_buffer_pointer] = ' ';
					keyboard_buffer_pointer++;

					if (keyboard_buffer_pointer <= 32) {
						keyboard_buffer[keyboard_buffer_pointer] = ch;
						keyboard_buffer_pointer++;
					}
				}
			}
			else {
				keyboard_buffer[keyboard_buffer_pointer] = ch;
				keyboard_buffer_pointer++;
			}
		}
	}
}

int load_ROM() {
	fstream ROM_file("ROM", ios::binary | ios::in);
	if (ROM_file.is_open()) {
		cout << "ROM file found, reading it..." << endl;
		ROM_file.seekg(0, ios::end);
		int ROM_file_size = ROM_file.tellg();
		ROM_file.seekg(0, ios::beg);
		if (ROM_file_size != 256) {
			cerr << "ROM file size is not 256 bytes!" << endl;
			return 1;
		}
		else {
			ROM_file.read(ROM, 256);
			cout << "reading ROM file was successful!" << endl;
		}
		ROM_file.close();
	}
	else {
		cout << "ROM file not found, attempting to create new ROM" << endl;
		fstream new_ROM_file("ROM", ios::binary | ios::out);
		if (new_ROM_file.is_open()) {
			char byte = 0;
			for (int i = 0; i < 256; i++) {
				new_ROM_file << byte;
				ROM[i] = 0;
			}
			cout << "new ROM file made successfully" << endl;
			new_ROM_file.close();
		}
		else {
			cerr << "Error: can't access ROM file" << endl;
			return 1;
		}
	}
	return 0;
}

int load_RAM() {
	fstream RAM_file("RAM", ios::binary | ios::in);
	if (RAM_file.is_open()) {
		cout << "RAM file found, reading it..." << endl;
		RAM_file.seekg(0, ios::end);
		int RAM_file_size = RAM_file.tellg();
		RAM_file.seekg(0, ios::beg);
		if (RAM_file_size != 65536) {
			cerr << "RAM file size is not 65536 bytes!" << endl;
			return 1;
		}
		else {
			RAM_file.read(RAM, 65536);
			cout << "reading RAM file was successful!" << endl;
		}
		RAM_file.close();
	}
	else {
		cout << "RAM file not found, attempting to create new RAM" << endl;
		fstream new_RAM_file("RAM", ios::binary | ios::out);
		if (new_RAM_file.is_open()) {
			char byte = 0;
			for (int i = 0; i < 65536; i++) {
				new_RAM_file << byte;
				RAM[i] = 0;
			}
			cout << "new RAM file made successfully" << endl;
			new_RAM_file.close();
		}
		else {
			cerr << "Error: can't access RAM file" << endl;
			return 1;
		}
	}
	return 0;
}

int load_STACK() {
	fstream STACK_file("STACK", ios::binary | ios::in);
	if (STACK_file.is_open()) {
		cout << "STACK file found, reading it..." << endl;
		STACK_file.seekg(0, ios::end);
		int ROM_file_size = STACK_file.tellg();
		STACK_file.seekg(0, ios::beg);
		if (ROM_file_size != 256) {
			cerr << "STACK file size is not 256 bytes!" << endl;
			return 1;
		}
		else {
			STACK_file.read(STACK, 256);
			cout << "reading STACK file was successful!" << endl;
		}
		STACK_file.close();
	}
	else {
		cout << "STACK file not found, attempting to create new STACK" << endl;
		fstream new_STACK_file("STACK", ios::binary | ios::out);
		if (new_STACK_file.is_open()) {
			char byte = 0;
			for (int i = 0; i < 256; i++) {
				new_STACK_file << byte;
				ROM[i] = 0;
			}
			cout << "new STACK file made successfully" << endl;
			new_STACK_file.close();
		}
		else {
			cerr << "Error: can't access STACK file" << endl;
			return 1;
		}
	}
	return 0;
}

void excute_instruction(unsigned char first_byte, unsigned char second_byte){
	unsigned char instruction_type = first_byte >> 3;
	unsigned char r0 = first_byte & 7, r1 = second_byte >> 5, r2 = (second_byte & 0b00011100) >> 2;
	if (instruction_type == Instruction_type::STR) {
		//set register r1 to low byte of the RAM address and r2 to its high byte, and store r0 in that address
		 RAM[(((short)reg[r2]) << 8) | ((short)reg[r1])] = reg[r0];
	}
	else if (instruction_type == Instruction_type::MOVI) {
		reg[r0] = second_byte;
	}
	else if (instruction_type == Instruction_type::MOV) {
		if (r0 != r1)
			reg[r1] = reg[r0];
	}
	else if (instruction_type == Instruction_type::ADD) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2)) {
			reg[r0] = reg[r1] + reg[r2];
			//if the value of the addition with 2 bytes is greater than the value of the addition with 1 byte
			if ((((short)reg[r1]) + ((short)reg[r2])) > ((short)reg[r0]))
				// 4d = 00000100b = the 3'rd bit (the carry bit) is set on in the flag register
				reg[7] = 4;
			//else if the value of the addition with 2 bytes is equal to the value of the addition with 1 byte
			else
				//the flags register is set to 0 to indicate that there were no flags set on in this operation
				reg[7] = 0;
		}
	}
	else if (instruction_type == Instruction_type::SUB) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
		//if the value of the subtraction with 2 bytes (to keep the same value but unsigned) is less than 0
		if ((((short)reg[r1]) - ((short)reg[r2])) < 0) {
			//set result to zero
			reg[r0] = 0;
			// 8d = 00001000b = the 4th bit (the borrow bit) is set on in the flag register
			reg[7] = 8;
		}
		//else if the value of the subtraction with 2 bytes (to keep the same value but unsigned) is equal to or greater than 0
		else {
			//perform subtraction normally
			reg[r0] = reg[r1] - reg[r2];
			//the flags register is set to 0 to indicate that there were no flags set on in this operation
			reg[7] = 0;
		}
	}
	else if (instruction_type == Instruction_type::MUL) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2)){
			reg[r0] = reg[r1] * reg[r2];
			//if the value of the multiplication with 2 bytes is greater than the value of the multiplication with 1 byte
			if ((((short)reg[r1]) * ((short)reg[r2])) > ((short)reg[r0]))
				// 16d = 00010000b = the 5th bit (the overflow bit) is set on in the flag register
				reg[7] = 16;
			//else if the value of the multiplication with 2 bytes is equal to the value of the multiplication with 1 byte
			else
				//the flags register is set to 0 to indicate that there were no flags set on in this operation
				reg[7] = 0;
		}

	}
	else if (instruction_type == Instruction_type::DIV) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = reg[r1] / reg[r2];
	}
	else if (instruction_type == Instruction_type::DIVRM) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = reg[r1] % reg[r2];
	}
	else if (instruction_type == Instruction_type::AND) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = reg[r1] & reg[r2];
	}
	else if (instruction_type == Instruction_type::OR) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = reg[r1] | reg[r2];
	}
	else if (instruction_type == Instruction_type::NOT) {
		if (r0 != r1)
			reg[r0] = ~reg[r1];
	}
	else if (instruction_type == Instruction_type::XOR) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = reg[r1] ^ reg[r2];
	}
	else if (instruction_type == Instruction_type::NAND) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = ~(reg[r1] & reg[r2]);
	}
	else if (instruction_type == Instruction_type::NOR) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = ~(reg[r1] | reg[r2]);
	}
	else if (instruction_type == Instruction_type::XNOR) {
		if ((r0 != r1) && (r0 != r2) && (r1 != r2))
			reg[r0] = ~(reg[r1] ^ reg[r2]);
	}
	else if (instruction_type == Instruction_type::SHFR) {
		if (r0 != r1){
			reg[r0] = reg[r1] >> 1;
		//if the first bit of the register shifted to the right is 1
		if ((reg[r1] & 1) == 1)
			// 16d = 00010000b = the 5th bit (the overflow bit) is set on in the flag register
			reg[7] = 16;
		//else if the first bit of the register shifted to the right is 0
		else
			//the flags register is set to 0 to indicate that there were no flags set on in this operation
			reg[7] = 0;
	}
	}
	else if (instruction_type == Instruction_type::SHFL) {
		if (r0 != r1){
			reg[r0] = reg[r1] << 1;
		//if the last bit  of the register shifted to the left is 1
		if ((reg[r1] & 128) == 1)
			// 16d = 00010000b = the 5th bit (the overflow bit) is set on in the flag register
			reg[7] = 16;
		//else if the last bit  of the register shifted to the left is 0
		else
			//the flags register is set to 0 to indicate that there were no flags set on in this operation
			reg[7] = 0;
		}
	}
	else if (instruction_type == Instruction_type::CMP) {
		if (r0 != r1) {
			if (reg[r0] > reg[r1])
			//32d = 00100000b; set the third to highest bit of the flag register on
				reg[7] = 32;
			else if (reg[r0] < reg[r1])
			//64d = 01000000b; set the second to highest bit of the flag register on
				reg[7] = 64;
			else
			//if(reg[r0] == reg[r1])
			//128d = 10000000b; set the highest bit of the flag register on
				reg[7] = 128;
		}
	}
	else if (instruction_type == Instruction_type::CMPI) {
			if (reg[r0] > second_byte)
				//32d = 00100000b; set the third to highest bit of the flag register on
				reg[7] = 32;
			else if (reg[r0] < second_byte)
				//64d = 01000000b; set the second to highest bit of the flag register on
				reg[7] = 64;
			else
				//if(reg[r0] == second_byte)
				//128d = 10000000b; set the highest bit of the flag register on
				reg[7] = 128;
	}
	else if (instruction_type == Instruction_type::CMPZ) {
			if (reg[r0] > 0)
				//32d = 00100000b; set the third to highest bit of the flag register on
				reg[7] = 32;
			else if (reg[r0] < 0)
				//64d = 01000000b; set the second to highest bit of the flag register on
				reg[7] = 64;
			else
				//if(reg[r0] == 0)
				//128d = 10000000b; set the highest bit of the flag register on
				reg[7] = 128;
	}
	else if (instruction_type == Instruction_type::PUSH) {
		STACK[stack_pointer] = reg[r0];
		if (stack_pointer == 255)
			stack_pointer = 0;
		else
			stack_pointer++;
	}
	else if (instruction_type == Instruction_type::PULL) {
		if (stack_pointer == 0) {
			reg[r0] = STACK[255];
			stack_pointer = 255;
		}
		else {
			reg[r0] = STACK[stack_pointer - 1];
			stack_pointer--;
		}
	}
	else if(instruction_type == Instruction_type::BEQ){
		if((r0 == 0) && (ROM_prgram_excution_mode)){
			//if flag register's highest bit (the equal flag bit) is 1
			if (((reg[7] & 128) >> 7) == 1) {
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if((r0 == 1) && (RAM_program_excution_mode)){
			//if flag register's highest bit (the equal flag bit) is 1
			if (((reg[7] & 128) >> 7) == 1) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if(instruction_type == Instruction_type::BNE){
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			//if flag register's highest bit (the equal flag bit) is 0
			if (((reg[7] & 128) >> 7) == 0) {
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//if flag register's highest bit (the equal flag bit) is 0
			if (((reg[7] & 128) >> 7) == 0) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if (instruction_type == Instruction_type::BGR) {
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			//if flag register's third to highest bit (the greater than flag bit) is 1
			if (((reg[7] & 32) >> 5) == 1) {
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//if flag register's third to highest bit (the greater than flag bit) is 1
			if (((reg[7] & 32) >> 5) == 1) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if (instruction_type == Instruction_type::BLE) {
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			//if flag register's second to highest bit (the less than flag bit) is 1
			if (((reg[7] & 64) >> 6) == 1) {
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//if flag register's second to highest bit (the less than flag bit) is 1
			if (((reg[7] & 64) >> 6) == 1) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if (instruction_type == Instruction_type::BNGR) {
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			//if flag register's third to highest bit (the greater than flag bit) is 0
			if (((reg[7] & 32) >> 5) == 0) {
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//if flag register's third to highest bit (the greater than flag bit) is 0
			if (((reg[7] & 32) >> 5) == 0) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if (instruction_type == Instruction_type::BNLE) {
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			//if flag register's second to highest bit (the less than flag bit) is 0
			if (((reg[7] & 64) >> 6) == 0){
				ROM_program_counter = second_byte;
				should_branch = true;
			}
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//if flag register's second to highest bit (the less than flag bit) is 0
			if (((reg[7] & 64) >> 6) == 0) {
				//set register r1 to low byte of the RAM counter and r2 to its high byte
				RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
				should_branch = true;
			}
		}
	}
	else if (instruction_type == Instruction_type::B) {
		if ((r0 == 0) && (ROM_prgram_excution_mode)) {
			ROM_program_counter = second_byte;
			should_branch = true;
		}
		else if ((r0 == 1) && (RAM_program_excution_mode)) {
			//set register r1 to low byte of the RAM counter and r2 to its high byte
			RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
			should_branch = true;
		}
	}
	else if (instruction_type == Instruction_type::LDR) {
		//set register r1 to low byte of the RAM address and r2 to its high byte, and load to r0 whar's in that address
		reg[r0] = RAM[(((short)reg[r2]) << 8) | ((short)reg[r1])];
	}
	else if (instruction_type == Instruction_type::IOCALL) {
		unsigned char ID = reg[r0];
		if (ID == IO_devices_IDs::TTY_DISPLAY) {
			//127d = 01111111b; we ignore the 8th bit
			if((reg[r1] & 127) != 0)
			putchar((char)(reg[r1] & 127));
		}
		else if (ID == IO_devices_IDs::KEYBOARD) {
			//127d = 01111111b; we ignore the 8th bit
			reg[r1] = (keyboard_buffer[keyboard_buffer_pointer] & 127);
			keyboard_buffer[keyboard_buffer_pointer] = 0;
			if (keyboard_buffer_pointer != 0) {
				keyboard_buffer_pointer--;
			}
		}
		else if (ID == IO_devices_IDs::RAM_program_excution_mode_ID) {
			ROM_prgram_excution_mode = false;
			RAM_program_excution_mode = true;
			//set register r1 to low byte of the RAM program counter and r2 to its high byte
			RAM_program_counter = (((short)reg[r2]) << 8) | ((short)reg[r1]);
		}
		else if (ID == IO_devices_IDs::ROM_program_excution_mode_ID) {
			ROM_prgram_excution_mode = true;
			RAM_program_excution_mode = false;
			ROM_program_counter = reg[r1];
		}
	}
}

void deallocate_reserved_memory_and_save() {
	delete[] ROM;
	fstream RAM_file("RAM", ios::binary | ios::out);
	if (RAM_file.is_open()) {
		RAM_file.write(RAM, 65536);
		RAM_file.close();
	}
	delete[] RAM;
	fstream STACK_file("STACK", ios::binary | ios::out);
	if (STACK_file.is_open()) {
		STACK_file.write(STACK, 256);
		STACK_file.close();
	}
	delete[] STACK;
}

void save_memory() {
	fstream RAM_file("RAM", ios::binary | ios::out);
	if (RAM_file.is_open()) {
		RAM_file.write(RAM, 65536);
		RAM_file.close();
	}
	fstream STACK_file("STACK", ios::binary | ios::out);
	if (STACK_file.is_open()) {
		STACK_file.write(STACK, 256);
		STACK_file.close();
	}
}

int main() {
	disable_echoing_user_input();
	std::thread keyboard_thread(check_for_keypresses);
	if (load_ROM() != 0)
		return 1;
	if (load_RAM() != 0)
		return 1;
	if (load_STACK() != 0)
		return 1;
	for (int i = 0; i < 32; i++)
		keyboard_buffer[i] = 0;

	atexit(deallocate_reserved_memory_and_save);

	chrono::steady_clock::time_point start;
	chrono::steady_clock::time_point end;

	start = std::chrono::high_resolution_clock::now();
	while (true) {
		if (ROM_prgram_excution_mode) {
			if (is_second_instruction_register_filled) {
				if (ROM_program_counter == 0){
					excute_instruction(ROM[255], ROM[0]);
				}
				else {
					excute_instruction(ROM[ROM_program_counter - 1], ROM[ROM_program_counter - 0]);
				}
				is_second_instruction_register_filled = false;
			}
			else
				is_second_instruction_register_filled = true;
			end = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(chrono::nanoseconds(100) - (start - end));
			start = std::chrono::high_resolution_clock::now();
			counter_for_save++;
			if (!should_branch) {
				if (ROM_program_counter == 255)
					ROM_program_counter = 0;
				else
					ROM_program_counter++;
			}
			else {
				should_branch = false;
				is_second_instruction_register_filled = false;
			}
			if (counter_for_save == 400) {
				save_memory();
				counter_for_save = 0;
			}
		}
		else if (RAM_program_excution_mode) {
			if (is_first_instruction_register_filled) {
				if (is_second_instruction_register_filled) {
					if (RAM_program_counter == 0) {
						excute_instruction(RAM[65535], RAM[0]);
					}
					else {
						excute_instruction(RAM[RAM_program_counter - 1], RAM[RAM_program_counter - 0]);
					}
					is_second_instruction_register_filled = false;
					is_first_instruction_register_filled = false;
				}
				else
					is_second_instruction_register_filled = true;
			}
			else
				is_first_instruction_register_filled = true;
			end = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(chrono::nanoseconds(100) - (start - end));
			start = std::chrono::high_resolution_clock::now();
			counter_for_save++;
			if (!(is_first_instruction_register_filled && is_second_instruction_register_filled)) {
				if (!should_branch) {
					if (RAM_program_counter == 65535)
						RAM_program_counter = 0;
					else
						RAM_program_counter++;
				}
				else {
					should_branch = false;
					is_first_instruction_register_filled = false;
					is_second_instruction_register_filled = false;
				}
			}
			if (counter_for_save == 400) {
				save_memory();
				counter_for_save = 0;
			}
		}
		else {
			cerr << "both \"RAM prgram excution mode\" and \"ROM prgram excution mode\" are false" << endl;
			break;
		}
	}
	return 0;
}
#include <cstring>
#include <iostream>
#include <format>
#include "skyr.h"

void skyr::chunk::write_byte(uint8_t byte)
{
	if (this->capacity < this->count + 1)
	{
		this->capacity = (this->capacity < 8) ? 8 : this->capacity * 2;
		uint8_t *old_code = this->code;
		this->code = new uint8_t[this->capacity];
		memcpy(this->code, old_code, this->count);
		delete old_code;
	}
	this->code[this->count++] = byte;
}

size_t skyr::chunk::dissassemble_instruction(size_t offset)
{
	std::cout << std::format("{:04x}", offset) << ' ';
	uint8_t instruction = this->code[offset];
	switch (instruction)
	{
		case skyr::OP_RETURN:
			std::cout << "RETURN\n";
			return offset + 1;
		default:
			std::cout << "Unknown\n";
			return offset + 1;
	}
}

void skyr::chunk::write_to_stdout(const char *name)
{
	std::cout << "====== " << name << " ======\n";
	for (size_t offset = 0; offset < this->count;)
	{
		offset = this->dissassemble_instruction(offset);
	}
}

skyr::chunk::~chunk()
{
	delete this->code;
}
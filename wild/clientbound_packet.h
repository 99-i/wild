#pragma once
#include <vector>
#include <string>

namespace write_fn
{
	std::vector<uint8_t> write_varint(int32_t varint);
}

struct lua_clientbound_packet
{
	std::vector <uint8_t> data;
	lua_clientbound_packet(int id);
	int id;
	void write_bool(bool b);
	void write_i8(int8_t i8);
	void write_u8(uint8_t u8);
	void write_i16(int16_t i16);
	void write_u16(uint16_t u16);
	void write_i32(int32_t i32);
	void write_i64(int64_t i64);
	void write_float(float f);
	void write_double(double d);
	void write_string(std::string s);
	void write_varint(int32_t varint);
};
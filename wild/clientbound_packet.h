#pragma once
#include "common.h"

namespace wild
{
	namespace write_fn
	{
		std::vector<uint8_t> write_bool(bool b);
		std::vector<uint8_t> write_i8(int8_t i8);
		std::vector<uint8_t> write_u8(uint8_t u8);
		std::vector<uint8_t> write_i16(int16_t i16);
		std::vector<uint8_t> write_u16(uint16_t u16);
		std::vector<uint8_t> write_i32(int32_t i32);
		std::vector<uint8_t> write_i64(int64_t i64);
		std::vector<uint8_t> write_float(float f);
		std::vector<uint8_t> write_double(double d);
		std::vector<uint8_t> write_string(std::string s);
		std::vector<uint8_t> write_varint(int32_t varint);
	}

	class clientbound_packet
	{
		std::vector <uint8_t> data;
		uint32_t id;
	public:
		clientbound_packet(uint32_t id);
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
		std::vector<uint8_t> package() const;
	};
}
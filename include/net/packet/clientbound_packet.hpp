#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wild
{
	namespace write_fn
	{
		std::vector<uint8_t> write_varint(int32_t varint);
		std::vector<uint8_t> write_i8(int8_t i8);
		std::vector<uint8_t> write_i16(int16_t i16);
		std::vector<uint8_t> write_i32(int32_t i32);
		std::vector<uint8_t> write_float(float f);
		std::vector<uint8_t> write_string(std::string string);
	} // namespace write_fn

	class clientbound_packet
	{
		friend class packet_builder;
		std::vector<uint8_t> data;
		uint32_t id;
		clientbound_packet(uint32_t id);
		void set_id(uint32_t id);
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
		// writes bytes WITHOUT PREFIXING WITH LENGTH.
		void write_bytes(const std::vector<uint8_t> &data);
		// a vector that contains all of the data that should be sent to the
		// client
		//(length, id, data)
	  public:
		std::vector<uint8_t> package() const;
	};

	class packet_builder
	{
	  private:
		clientbound_packet packet;

	  public:
		packet_builder(uint32_t id);
		packet_builder();
		packet_builder &set_id(uint32_t id);
		packet_builder &append_bool(bool b);
		packet_builder &append_i8(int8_t i8);
		packet_builder &append_u8(uint8_t u8);
		packet_builder &append_i16(int16_t i16);
		packet_builder &append_u16(uint16_t u16);
		packet_builder &append_i32(int32_t i32);
		packet_builder &append_i64(int64_t i64);
		packet_builder &append_float(float f);
		packet_builder &append_double(double d);
		packet_builder &append_string(std::string s,
									  bool prepend_length = true);
		packet_builder &append_varint(int32_t varint);
		packet_builder &append_bytes(const std::vector<uint8_t> &data);
		packet_builder &append_bytes(const uint8_t *const buffer,
									 const size_t len);

		clientbound_packet build() const;
	};
} // namespace wild

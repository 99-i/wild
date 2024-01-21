#pragma once
#include "../../common.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

// the packet struct is solely for INCOMING packets.
// OUTGOING packets are constructed with wild::clientbound_packet
namespace wild
{
	enum class data_type
	{
		BOOL,
		BYTE,
		UNSIGNED_BYTE,
		SHORT,
		UNSIGNED_SHORT,
		INT,
		LONG,
		FLOAT,
		DOUBLE,
		STRING,
		VARINT,
		// BYTE_ARRAY,
		// SLOT
	};

	// different types of packets, their states, and their fields.
	struct packet_form
	{
		std::string_view name;
		client_state state;
		int id;
		int num_fields;

		struct field
		{
			std::string_view name;
			data_type type;
		} fields[7]; // 7 is the highest amount of fields a serverbound packet
					 // can have todo: change fields to std::vector? lots of
					 // unused memory
	};

	// defined in packet_forms.cpp
	constexpr int FORMS_SIZE = 40;
	extern packet_form forms[FORMS_SIZE];

	// a serverbound packet
	struct packet
	{
		typedef std::variant<bool, uint8_t, int8_t, uint16_t, int16_t, int32_t,
							 int64_t, float, double, std::string,
							 std::vector<uint8_t>>
			packet_field_t;
		std::string_view name;
		uint32_t id;
		std::map<std::string, packet_field_t> data;
	};
	// which bits to read in a varint byte (0b0111 1111)
	constexpr uint8_t VARINT_READ_BITS = 0x7F;
	// which bit to check if to continue to the next byte in a varint (0b1000
	// 0000)
	constexpr uint8_t VARINT_CONTINUE_BITS = 0x80;

	namespace read_fn
	{
		// reads a varint sequentially without having to have all the bytes
		// once. useful for reading the length varint at the start of every
		// packet.
		struct varint_reader
		{
			int32_t value = 0;
			int position = 0;
			// return value: if value contains the full value (e.g. continue bit
			// was 0.)
			bool push_byte(uint8_t byte)
			{
				this->value |= (byte & VARINT_READ_BITS) << this->position;

				if ((byte & (VARINT_CONTINUE_BITS)) == 0)
					return true;

				this->position += 7;

				return false;
			}

			void reset()
			{
				this->value = 0;
				this->position = 0;
			}
		};

		// if these return nullopt, the packet data was misformed and the client
		// is kicked.
		std::optional<bool>
		read_bool(std::vector<uint8_t>::iterator &data,
				  const std::vector<uint8_t>::iterator &end);
		std::optional<int8_t>
		read_i8(std::vector<uint8_t>::iterator &data,
				const std::vector<uint8_t>::iterator &end);
		std::optional<uint8_t>
		read_u8(std::vector<uint8_t>::iterator &data,
				const std::vector<uint8_t>::iterator &end);
		std::optional<int16_t>
		read_i16(std::vector<uint8_t>::iterator &data,
				 const std::vector<uint8_t>::iterator &end);
		std::optional<uint16_t>
		read_u16(std::vector<uint8_t>::iterator &data,
				 const std::vector<uint8_t>::iterator &end);
		std::optional<int32_t>
		read_i32(std::vector<uint8_t>::iterator &data,
				 const std::vector<uint8_t>::iterator &end);
		std::optional<int64_t>
		read_i64(std::vector<uint8_t>::iterator &data,
				 const std::vector<uint8_t>::iterator &end);
		std::optional<float>
		read_float(std::vector<uint8_t>::iterator &data,
				   const std::vector<uint8_t>::iterator &end);
		std::optional<double>
		read_double(std::vector<uint8_t>::iterator &data,
					const std::vector<uint8_t>::iterator &end);

		std::optional<std::vector<uint8_t>>
		read_byte_array(size_t array_size, std::vector<uint8_t>::iterator &data,
						const std::vector<uint8_t>::iterator &end);

		std::optional<std::string>
		read_string(std::vector<uint8_t>::iterator &data,
					const std::vector<uint8_t>::iterator &end);
		std::optional<int32_t>
		read_varint(std::vector<uint8_t>::iterator &data,
					const std::vector<uint8_t>::iterator &end);
	} // namespace read_fn
} // namespace wild

#pragma once
#include <vector>
#include <string_view>
#include <map>
#include <variant>
#include <optional>

#include "client.h"

//the packet struct is solely for INCOMING packets.
//OUTGOING packets are constructed in lua in the main libraries that are provided.

enum class w_data_type
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
	//BYTE_ARRAY,
	//SLOT
};

struct w_packet_form
{
	std::string_view name;
	w_client::client_state state;
	int id;
	int num_fields;
	struct field
	{
		std::string_view name;
		w_data_type type;
	} fields[7];
};

constexpr int FORMS_SIZE = 40;
extern w_packet_form forms[FORMS_SIZE];

struct w_packet
{
	typedef std::variant<bool, uint8_t, int8_t, uint16_t, int16_t, int32_t, int64_t, float, double, std::string> packet_field_t;
	std::string_view name;
	const w_packet_form *form;
	std::map<std::string, packet_field_t> data;
};
constexpr uint8_t VARINT_READ_BITS = 0x7F;
constexpr uint8_t VARINT_CONTINUE_BITS = 0x80;

namespace read_fn
{
	//if this returns a non-option, the packet data was misformed and the client is kicked.
	std::optional<bool> read_bool(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<int8_t> read_i8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<uint8_t> read_u8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<int16_t> read_i16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<uint16_t> read_u16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<int32_t> read_i32(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<int64_t> read_i64(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<float> read_float(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<double> read_double(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<std::string> read_string(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
	std::optional<int32_t> read_varint(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end);
}

struct w_packet_read_stream
{
private:
	enum
	{
		// haven't gotten any new packet data/just finished a packet
		BEGIN,
		//in the middle of reading the length varint
		READ_LENGTH_VARINT,
		//waiting for all the data to be sent so we can read it.
		WAITING_FOR_ALL_DATA
	} read_state = BEGIN;

	struct varint_reader
	{
		int32_t value = 0;
		int position = 0;
		//return value: if value contains the full value (e.g. continue bit was 0.)
		bool push_byte(uint8_t byte)
		{
			this->value |= (byte & VARINT_READ_BITS) << this->position;

			if ((byte & (VARINT_CONTINUE_BITS)) == 0) return true;

			this->position += 7;

			return false;
		}

		void reset()
		{
			this->value = 0;
			this->position = 0;
		}
	};

	varint_reader length_reader;

	//goes down every time a byte is read. if 0, throw error.
	int length_remaining = 0;

	//the buffer of data that gets written to while we're waiting for the whole thing to be finished sending.
	std::vector<uint8_t> buffer;

	w_client *client;

	//return true if data was handled correctly.
	//return false if malformed data from client.
	bool handle_data(uint8_t byte);
	void reset();

	//return true if data was handled correctly.
	//return false if malformed data from client.
	bool read_packet();

public:

	//return true if data was handled correctly.
	//return false if malformed data from client.
	bool handle_data(const std::vector<uint8_t> &data);
	w_packet_read_stream(w_client *client);
};

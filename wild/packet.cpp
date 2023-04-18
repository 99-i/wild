#include <vector>
#include "packet.h"
#include "server.h"
#include <plog/Log.h>
bool w_packet_read_stream::handle_data(uint8_t byte)
{
	bool ret = true;
	switch (this->read_state)
	{
		//this is a byte of the length varint.
		case BEGIN:
		case READ_LENGTH_VARINT:
		{
			if (this->length_reader.push_byte(byte))
			{
				this->read_state = WAITING_FOR_ALL_DATA;
				this->length_remaining = this->length_reader.value;
			}
			break;
		}
		case WAITING_FOR_ALL_DATA:
		{
			this->length_remaining--;
			this->buffer.push_back(byte);

			if (this->length_remaining <= 0)
			{
				ret = this->read_packet();
				this->reset();
			}
		}
	}
	return ret;
}

void w_packet_read_stream::reset()
{
	this->read_state = BEGIN;
	this->length_reader.reset();
	this->length_remaining = 0;
	this->buffer.clear();
}

bool w_packet_read_stream::read_packet()
{
	auto needle = this->buffer.begin();
	auto end = this->buffer.end();
	std::optional<int32_t> id = read_fn::read_varint(needle, end);
	if (!id.has_value())
		return false;

	const w_packet_form *form;
	bool found_form = false;
	for (int i = 0; i < FORMS_SIZE; i++)
	{
		form = &forms[i];
		if (form->id == id && this->client->state == form->state)
		{
			found_form = true;
			break;
		}
	}
	if (!found_form)
	{
		//KICK_CLIENT;
		return true;
	}

	w_packet *packet = new w_packet();
	packet->name = form->name;
	packet->form = form;
	for (int i = 0; i < form->num_fields; i++)
	{
		w_packet_form::field field = form->fields[i];
		switch (field.type)
		{
			case w_data_type::BOOL:
			{
				auto res = read_fn::read_bool(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::BYTE:
			{
				auto res = read_fn::read_i8(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::UNSIGNED_BYTE:
			{
				auto res = read_fn::read_u8(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::SHORT:
			{
				auto res = read_fn::read_i16(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::UNSIGNED_SHORT:
			{
				auto res = read_fn::read_u16(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::INT:
			{
				auto res = read_fn::read_i32(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::LONG:
			{
				auto res = read_fn::read_i64(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::FLOAT:
			{
				auto res = read_fn::read_float(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::DOUBLE:
			{
				auto res = read_fn::read_double(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::STRING:
			{
				auto res = read_fn::read_string(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			case w_data_type::VARINT:
			{
				auto res = read_fn::read_varint(needle, end);
				if (!res.has_value())
					return false;

				packet->data.emplace(field.name, res.value());
			}
			break;
			default:
				break;
		}
	}

	this->client->server->handle_client_packet(this->client, packet);
	return true;
}

//handle incoming data for this client.
bool w_packet_read_stream::handle_data(const std::vector<uint8_t> &data)
{
	bool dc = true;
	for (int i = 0; i < data.size(); i++)
	{
		if (!this->handle_data(data[i]))
		{
			return true;
		}
	}
}

w_packet_read_stream::w_packet_read_stream(w_client *client) : client(client)
{
}

std::optional<bool> read_fn::read_bool(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (bool)*data++;
}

std::optional<int8_t> read_fn::read_i8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (int8_t)*data++;
}

std::optional<uint8_t> read_fn::read_u8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (uint8_t)*data++;
}

std::optional<int16_t> read_fn::read_i16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data + 2 > end) return std::nullopt;

	return (*data++ << 8) | *data++;
}

std::optional<uint16_t> read_fn::read_u16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data + 2 > end) return std::nullopt;

	return (*data++ << 8) | *data++;
}

std::optional<int32_t> read_fn::read_i32(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data + 4 > end) return std::nullopt;

	return (*data++ << 24) | (*data++ << 16) | (*data++ << 8) | *data++;
}

std::optional<int64_t> read_fn::read_i64(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data + 8 > end) return std::nullopt;
	int64_t n = 0;
	return (((uint64_t)*data++) << 56) | (((uint64_t)*data++) << 48) | (((uint64_t)*data++) << 40) | (((uint64_t)*data++) << 32) | (((uint64_t)*data++) << 24) | (((uint64_t)*data++) << 16) | (((uint64_t)*data++) << 8) | ((uint64_t)*data++);
}

std::optional<float> read_fn::read_float(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int32_t> bit_form = read_fn::read_i32(data, end);
	if (!bit_form.has_value()) return std::nullopt;

	return std::bit_cast<float>(bit_form.value());
}

std::optional<double> read_fn::read_double(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int64_t> bit_form = read_fn::read_i64(data, end);
	if (!bit_form.has_value()) return std::nullopt;

	return std::bit_cast<double>(bit_form.value());
}

std::optional<std::string> read_fn::read_string(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int32_t> length = read_fn::read_varint(data, end);
	if (!length.has_value()) return std::nullopt;

	if (data + length.value() > end) return std::nullopt;

	std::string result = std::string(data, data + length.value());
	data += length.value();

	return result;
}

std::optional<int32_t> read_fn::read_varint(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	int32_t value = 0;
	int position = 0;
	while (1)
	{
		if (data == end)
		{
			return std::nullopt;
		}
		value |= (*data & VARINT_READ_BITS) << position;
		if ((*data++ & VARINT_CONTINUE_BITS) == 0) break;
		position += 7;
		if (position >= 32) return std::nullopt;
	}
	return value;
}
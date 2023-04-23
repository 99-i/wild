#include <vector>
#include "packet.h"
#include "server.h"
#include <plog/Log.h>

std::optional<bool> wild::read_fn::read_bool(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (bool)*data++;
}

std::optional<int8_t> wild::read_fn::read_i8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (int8_t)*data++;
}

std::optional<uint8_t> wild::read_fn::read_u8(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	if (data == end) return std::nullopt;
	return (uint8_t)*data++;
}
#define RETURN_NULL_ON_SIZE_REQUIREMENT(s) if(std::distance(data, end) < s) return std::nullopt
std::optional<int16_t> wild::read_fn::read_i16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	RETURN_NULL_ON_SIZE_REQUIREMENT(2);
	return (*data++ << 8) | *data++;
}

std::optional<uint16_t> wild::read_fn::read_u16(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	RETURN_NULL_ON_SIZE_REQUIREMENT(2);
	return (*data++ << 8) | *data++;
}

std::optional<int32_t> wild::read_fn::read_i32(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	RETURN_NULL_ON_SIZE_REQUIREMENT(4);
	return (*data++ << 24) | (*data++ << 16) | (*data++ << 8) | *data++;
}

std::optional<int64_t> wild::read_fn::read_i64(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	RETURN_NULL_ON_SIZE_REQUIREMENT(8);
	int64_t n = 0;
	return (((uint64_t)*data++) << 56) | (((uint64_t)*data++) << 48) | (((uint64_t)*data++) << 40) | (((uint64_t)*data++) << 32) | (((uint64_t)*data++) << 24) | (((uint64_t)*data++) << 16) | (((uint64_t)*data++) << 8) | ((uint64_t)*data++);
}

std::optional<float> wild::read_fn::read_float(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int32_t> bit_form = wild::read_fn::read_i32(data, end);
	if (!bit_form.has_value()) return std::nullopt;

	return std::bit_cast<float>(bit_form.value());
}

std::optional<double> wild::read_fn::read_double(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int64_t> bit_form = wild::read_fn::read_i64(data, end);
	if (!bit_form.has_value()) return std::nullopt;

	return std::bit_cast<double>(bit_form.value());
}

std::optional<std::string> wild::read_fn::read_string(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
{
	std::optional<int32_t> length = wild::read_fn::read_varint(data, end);
	if (!length.has_value()) return std::nullopt;

	RETURN_NULL_ON_SIZE_REQUIREMENT(length.value());

	std::string result = std::string(data, data + length.value());
	data += length.value();

	return result;
}

std::optional<int32_t> wild::read_fn::read_varint(std::vector<uint8_t>::iterator &data, const std::vector<uint8_t>::iterator &end)
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
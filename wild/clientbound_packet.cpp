#include "clientbound_packet.h"
#include "packet.h"

wild::clientbound_packet::clientbound_packet(uint32_t id) : id(id)
{
}
void wild::clientbound_packet::write_bool(bool b)
{
	this->data.push_back(b);
}
void wild::clientbound_packet::write_i8(int8_t i8)
{
	this->data.push_back(i8);
}
void wild::clientbound_packet::write_u8(uint8_t u8)
{
	this->data.push_back(u8);
}
void wild::clientbound_packet::write_i16(int16_t i16)
{
	this->data.push_back((i16 & 0xff00) >> 8);
	this->data.push_back(i16 & 0xff);
}
void wild::clientbound_packet::write_u16(uint16_t u16)
{
	this->data.push_back((u16 & 0xff00) >> 8);
	this->data.push_back(u16 & 0xff);
}
void wild::clientbound_packet::write_i32(int32_t i32)
{
	this->data.push_back((i32 & 0xff000000) >> 24);
	this->data.push_back((i32 & 0xff0000) >> 16);
	this->data.push_back((i32 & 0xff00) >> 8);
	this->data.push_back((i32 & 0xff));
}
void wild::clientbound_packet::write_i64(int64_t i64)
{
	this->data.push_back((i64 & 0xff00000000000000) >> 56);
	this->data.push_back((i64 & 0xff000000000000) >> 48);
	this->data.push_back((i64 & 0xff0000000000) >> 40);
	this->data.push_back((i64 & 0xff00000000) >> 32);
	this->data.push_back((i64 & 0xff000000) >> 24);
	this->data.push_back((i64 & 0xff0000) >> 16);
	this->data.push_back((i64 & 0xff00) >> 8);
	this->data.push_back((i64 & 0xff));
}
void wild::clientbound_packet::write_float(float f)
{
	int32_t temp = std::bit_cast<int32_t>(f);
	this->write_i32(temp);
}
void wild::clientbound_packet::write_double(double d)
{
	int64_t temp = std::bit_cast<int64_t>(d);
	this->write_i64(temp);
}
void wild::clientbound_packet::write_string(std::string s)
{
	this->write_varint(s.size());
	for (char c : s)
	{
		this->write_u8(c);
	}
}
void wild::clientbound_packet::write_varint(int32_t varint)
{
	std::vector<uint8_t> varint_data = write_fn::write_varint(varint);
	this->data.insert(this->data.end(), varint_data.begin(), varint_data.end());
}

std::vector<uint8_t> wild::clientbound_packet::package() const
{
	std::vector<uint8_t> id_varint = write_fn::write_varint(this->id);
	std::vector<uint8_t> length_varint = write_fn::write_varint(id_varint.size() + this->data.size());

	std::vector<uint8_t> all;
	all.insert(all.begin(), length_varint.begin(), length_varint.end());
	all.insert(all.end(), id_varint.begin(), id_varint.end());
	all.insert(all.end(), this->data.begin(), this->data.end());
	return all;
}

std::vector<uint8_t> wild::write_fn::write_varint(int32_t varint)
{
	std::vector<uint8_t> data;
	int i = 0;
	while (i != 5)
	{
		if ((varint & ~VARINT_READ_BITS) == 0)
		{
			data.push_back(varint);
			break;
		}

		data.push_back((varint & VARINT_READ_BITS) | VARINT_CONTINUE_BITS);
		varint >>= 7;
		i++;
	}

	return data;
}
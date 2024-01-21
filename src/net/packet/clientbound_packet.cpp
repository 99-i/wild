#include "net/packet/clientbound_packet.hpp"
#include "net/packet/packet.hpp"

namespace wild
{
	clientbound_packet::clientbound_packet(uint32_t id) : id(id) {}
	void clientbound_packet::set_id(uint32_t id) { this->id = id; }
	void clientbound_packet::write_bool(bool b) { this->data.push_back(b); }
	void clientbound_packet::write_i8(int8_t i8) { this->data.push_back(i8); }
	void clientbound_packet::write_u8(uint8_t u8) { this->data.push_back(u8); }
	void clientbound_packet::write_i16(int16_t i16)
	{
		this->data.push_back((i16 & 0xff00) >> 8);
		this->data.push_back(i16 & 0xff);
	}
	void clientbound_packet::write_u16(uint16_t u16)
	{
		this->data.push_back((u16 & 0xff00) >> 8);
		this->data.push_back(u16 & 0xff);
	}
	void clientbound_packet::write_i32(int32_t i32)
	{
		this->data.push_back((i32 & 0xff000000) >> 24);
		this->data.push_back((i32 & 0xff0000) >> 16);
		this->data.push_back((i32 & 0xff00) >> 8);
		this->data.push_back((i32 & 0xff));
	}
	void clientbound_packet::write_i64(int64_t i64)
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
	void clientbound_packet::write_float(float f)
	{
		int32_t temp = std::bit_cast<int32_t>(f);
		this->write_i32(temp);
	}
	void clientbound_packet::write_double(double d)
	{
		int64_t temp = std::bit_cast<int64_t>(d);
		this->write_i64(temp);
	}
	void clientbound_packet::write_string(std::string s)
	{
		this->write_varint(s.size());
		for (char c : s)
			{
				this->write_u8(c);
			}
	}
	void clientbound_packet::write_varint(int32_t varint)
	{
		std::vector<uint8_t> varint_data = write_fn::write_varint(varint);
		this->data.insert(this->data.end(), varint_data.begin(),
						  varint_data.end());
	}

	void clientbound_packet::write_bytes(const std::vector<uint8_t> &data)
	{
		this->data.insert(this->data.end(), data.begin(), data.end());
	}

	std::vector<uint8_t> clientbound_packet::package() const
	{
		std::vector<uint8_t> id_varint = write_fn::write_varint(this->id);
		std::vector<uint8_t> length_varint =
			write_fn::write_varint(id_varint.size() + this->data.size());

		std::vector<uint8_t> all;
		all.insert(all.begin(), length_varint.begin(), length_varint.end());
		all.insert(all.end(), id_varint.begin(), id_varint.end());
		all.insert(all.end(), this->data.begin(), this->data.end());
		return all;
	}

	packet_builder::packet_builder(uint32_t id) : packet(id) {}
	packet_builder::packet_builder() : packet(0) {}

	packet_builder &packet_builder::set_id(uint32_t id)
	{
		this->packet.set_id(id);
		return *this;
	}
	packet_builder &packet_builder::append_bool(bool b)
	{
		this->packet.write_bool(b);
		return *this;
	}
	packet_builder &packet_builder::append_i8(int8_t i8)
	{
		this->packet.write_i8(i8);
		return *this;
	}
	packet_builder &packet_builder::append_u8(uint8_t u8)
	{
		this->packet.write_u8(u8);
		return *this;
	}
	packet_builder &packet_builder::append_i16(int16_t i16)
	{
		this->packet.write_i16(i16);
		return *this;
	}
	packet_builder &packet_builder::append_u16(uint16_t u16)
	{
		this->packet.write_u16(u16);
		return *this;
	}
	packet_builder &packet_builder::append_i32(int32_t i32)
	{
		this->packet.write_i32(i32);
		return *this;
	}
	packet_builder &packet_builder::append_i64(int64_t i64)
	{
		this->packet.write_i64(i64);
		return *this;
	}
	packet_builder &packet_builder::append_float(float f)
	{
		this->packet.write_float(f);
		return *this;
	}
	packet_builder &packet_builder::append_double(double d)
	{
		this->packet.write_double(d);
		return *this;
	}
	packet_builder &packet_builder::append_string(std::string s,
												  bool prepend_length)
	{
		if (!prepend_length)
			{
				std::vector<uint8_t> vec(s.begin(), s.end());
				this->packet.write_bytes(vec);
			}
		else
			{
				this->packet.write_string(s);
			}
		return *this;
	}
	packet_builder &packet_builder::append_varint(int32_t varint)
	{
		this->packet.write_varint(varint);
		return *this;
	}
	packet_builder &
	packet_builder::append_bytes(const std::vector<uint8_t> &data)
	{
		this->packet.write_bytes(data);
		return *this;
	}
	packet_builder &packet_builder::append_bytes(const uint8_t *const buffer,
												 const size_t len)
	{
		std::vector<uint8_t> buf = std::vector<uint8_t>(buffer, buffer + len);
		this->packet.write_bytes(buf);
		return *this;
	}

	clientbound_packet packet_builder::build() const { return this->packet; }

	std::vector<uint8_t> write_fn::write_varint(int32_t varint)
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

				data.push_back((varint & VARINT_READ_BITS) |
							   VARINT_CONTINUE_BITS);
				varint >>= 7;
				i++;
			}

		return data;
	}

	std::vector<uint8_t> write_fn::write_i8(int8_t i8)
	{
		std::vector<uint8_t> data;
		data.push_back(i8);
		return data;
	}

	std::vector<uint8_t> write_fn::write_i16(int16_t i16)
	{
		std::vector<uint8_t> data;
		data.push_back((i16 & 0xff00) >> 8);
		data.push_back(i16 & 0xff);
		return data;
	}

	std::vector<uint8_t> write_fn::write_i32(int32_t i32)
	{
		std::vector<uint8_t> data;
		data.push_back((i32 & 0xff000000) >> 24);
		data.push_back((i32 & 0xff0000) >> 16);
		data.push_back((i32 & 0xff00) >> 8);
		data.push_back((i32 & 0xff));
		return data;
	}

	std::vector<uint8_t> write_fn::write_float(float f)
	{
		int32_t temp = std::bit_cast<int32_t>(f);
		return write_fn::write_i32(temp);
	}

	std::vector<uint8_t> write_fn::write_string(std::string string)
	{
		std::vector<uint8_t> data = write_fn::write_varint(string.size());
		for (char c : string)
			{
				data.push_back(c);
			}
		return data;
	}
} // namespace wild

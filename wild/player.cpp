#include <zlib.h>
#include "clientbound_packet.h"
#include "client.h"
#include "common.h"
#include "nibble_vector.h"
#include "player.h"

wild::player::player(wild::client &client) : _client(client)
{
	this->health = 20.0f;
}

wild::metadata::human_metadata wild::player::get_metadata() const
{
	wild::metadata::human_metadata metadata;
	metadata.set_health(this->health);

	return metadata;
}

void wild::player::send_join_game_packet(uint8_t gamemode, int8_t dimension, uint8_t difficulty, uint8_t max_players, std::string level_type)
{
	auto join_game = packet_builder(0x01)
		.append_i32(this->id)
		.append_u8(gamemode)
		.append_i8(dimension)
		.append_u8(difficulty)
		.append_u8(max_players)
		.append_string(level_type)
		.build();
	this->_client.send_packet(join_game);
}
void wild::player::send_spawn_position_packet()
{
	//todo
	auto spawn_position = packet_builder(0x05)
		.append_i32(0)
		.append_i32(0)
		.append_i32(0)
		.build();
	this->_client.send_packet(spawn_position);
}
void wild::player::send_player_abilities_packet()
{
	//todo
	auto player_abilities = packet_builder(0x39)
		.append_i8(0b00000100)
		.append_float(0.2f)
		.append_float(0.2f)
		.build();
	this->_client.send_packet(player_abilities);
}

void wild::player::send_move_and_look_packet()
{
	//todo
	auto player_position_and_look = packet_builder(0x08)
		.append_double(this->pos.x)
		.append_double(this->pos.y)
		.append_double(this->pos.z)
		.append_float(this->pos.yaw)
		.append_float(this->pos.pitch)
		.append_bool(false) //todo: check if on ground.
		.build();
	this->_client.send_packet(player_position_and_look);
}
void wild::player::send_spawn_player_packet(const wild::player &other)
{
	//todo
	auto spawn_player = packet_builder(0x0c)
		.append_varint(other.id)
		.append_string("776e5cb3-66a5-48b0-9a0f-c29bb888c712")
		.append_string(other._username)
		.append_varint(0)
		.append_i32(wild::fixed_point<int32_t, 5>(other.pos.x).raw_data())
		.append_i32(wild::fixed_point<int32_t, 5>(other.pos.y).raw_data())
		.append_i32(wild::fixed_point<int32_t, 5>(other.pos.z).raw_data())
		.append_i8(0)
		.append_i8(0)
		.append_i16(0)
		.append_bytes(other.get_metadata().get_data())
		.build();

	this->_client.send_packet(spawn_player);
}

void compress_buffer(const std::vector<uint8_t> &source, std::vector<uint8_t> &dest)
{
	uint32_t compressed_length = compressBound(source.size());
	uint8_t *dest_buf = new uint8_t[compressed_length];
	compress(dest_buf, (uLongf *)&compressed_length, source.data(), source.size());

	std::vector<uint8_t> data_vector(dest_buf, dest_buf + compressed_length);
	dest.reserve(compressed_length);
	dest.insert(dest.begin(), data_vector.begin(), data_vector.end());
}
void wild::player::send_bulk_chunk_data_packet()
{
	std::vector<uint8_t> chunk_data;
	wild::nibble_vector metadata;
	wild::nibble_vector block_light;
	wild::nibble_vector sky_light;
	wild::nibble_vector add_array;
	for (int y = 0; y < 16; y++)
	{
		for (int z = 0; z < 16; z++)
		{
			for (int x = 0; x < 16; x++)
			{
				chunk_data.push_back(y);
				metadata.push(0);
				block_light.push(0);
				sky_light.push(0);
				add_array.push(0);
			}
		}
	}
	chunk_data.insert(chunk_data.end(), metadata.data.begin(), metadata.data.end());
	chunk_data.insert(chunk_data.end(), block_light.data.begin(), block_light.data.end());
	chunk_data.insert(chunk_data.end(), sky_light.data.begin(), sky_light.data.end());
	chunk_data.insert(chunk_data.end(), add_array.data.begin(), add_array.data.end());

	for (int i = 0; i < 256; i++)
	{
		chunk_data.push_back(1);
	}

	std::vector<uint8_t> compressed_chunk_data;

	compress_buffer(chunk_data, compressed_chunk_data);

	auto chunk_data_packet = packet_builder(0x21)
		.append_i32(0) //chunk x
		.append_i32(0) //chunk z
		.append_bool(true) //ground-up continuous
		.append_u16(0b00000001) //primary bit map
		.append_u16(0) //add bit map
		.append_i32(compressed_chunk_data.size())
		.append_bytes(compressed_chunk_data)
		.build();

	this->_client.send_packet(chunk_data_packet);
}
void wild::player::send_entity_relative_look(const wild::entity &entity, float yaw, float pitch)
{
	auto entity_look = packet_builder(0x16)
		.append_i32(entity.id)
		.append_i8((int8_t)(yaw * 256.f / 360.f))
		.append_i8((int8_t)(pitch * 256.f / 360.f))
		.build();

	this->_client.send_packet(entity_look);
}
void wild::player::send_entity_relative_move(const wild::entity &entity, float dx, float dy, float dz)
{
	int8_t dx_fp = wild::fixed_point<int8_t, 5>(dx).raw_data();
	int8_t dy_fp = wild::fixed_point<int8_t, 5>(dy).raw_data();
	int8_t dz_fp = wild::fixed_point<int8_t, 5>(dz).raw_data();

	auto entity_relative_move = packet_builder(0x15)
		.append_i32(entity.id)
		.append_i8(dx_fp)
		.append_i8(dy_fp)
		.append_i8(dz_fp)
		.build();

	this->_client.send_packet(entity_relative_move);
}
void wild::player::send_entity_relative_move_and_look(const wild::entity &entity, float dx, float dy, float dz, float yaw, float pitch)
{
	int8_t dx_fp = wild::fixed_point<int8_t, 5>(dx).raw_data();
	int8_t dy_fp = wild::fixed_point<int8_t, 5>(dy).raw_data();
	int8_t dz_fp = wild::fixed_point<int8_t, 5>(dz).raw_data();

	auto entity_look_and_relative_move = packet_builder(0x17)
		.append_i32(entity.id)
		.append_i8(dx_fp)
		.append_i8(dy_fp)
		.append_i8(dz_fp)
		.append_i8((int8_t)(yaw * 256.f / 360.f))
		.append_i8((int8_t)(pitch * 256.f / 360.f))
		.build();
	this->_client.send_packet(entity_look_and_relative_move);
}
void wild::player::send_entity_packet(const wild::entity &entity)
{
	auto entity_packet = packet_builder(0x14)
		.append_i32(entity.id)
		.build();

	this->_client.send_packet(entity_packet);
}
void wild::player::send_entity_head_look(const wild::entity &entity, float yaw)
{
	auto entity_head_look = packet_builder(0x19)
		.append_i32(entity.id)
		.append_i8((int8_t)(yaw * 256.f / 360.f))
		.build();
	this->_client.send_packet(entity_head_look);
}
#include "clientbound_packet.h"
#include "client.h"
#include "player.h"
#include "common.h"

wild::player::player(wild::client &client) : _client(client)
{
	this->metadata.set_health(20.0f);
}

wild::client &wild::player::client() const
{
	return this->_client;
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
		.append_i8(0b00000111)
		.append_float(1.0f / 250.0f)
		.append_float(1.0f / 250.0f)
		.build();
	this->_client.send_packet(player_abilities);
}
void wild::player::send_position_packet()
{
	//todo
	auto player_position_and_look = packet_builder(0x08)
		.append_double(this->pos.x)
		.append_double(this->pos.y)
		.append_double(this->pos.z)
		.append_float(0.0)
		.append_float(0.0)
		.append_bool(true)
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
		.append_i32(0)
		.append_i32(0)
		.append_i32(0)
		.append_i8(0)
		.append_i8(0)
		.append_i16(0)
		.append_bytes(other.metadata.get_data())
		.build();

	this->_client.send_packet(spawn_player);
}
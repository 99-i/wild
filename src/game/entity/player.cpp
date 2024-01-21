#include "game/entity/player.hpp"
#include "net/client.hpp"
#include "net/packet/clientbound_packet.hpp"
#include <plog/Log.h>
#include <zlib.h>

namespace wild
{
	player::player(client &client)
		: _client(client), living_entity(entity_type::PLAYER)
	{
		this->health = 20.0f;
	}

	metadata::human_metadata player::get_metadata() const
	{
		metadata::human_metadata metadata;
		metadata.set_health(this->health);

		return metadata;
	}

	bool player::send_join_game_packet(uint8_t gamemode, int8_t dimension,
									   uint8_t difficulty, uint8_t max_players,
									   std::string level_type)
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
		return true;
	}

	bool player::send_spawn_position_packet()
	{
		// todo
		auto spawn_position = packet_builder(0x05)
								  .append_i32(0)
								  .append_i32(0)
								  .append_i32(0)
								  .build();
		this->_client.send_packet(spawn_position);
		return true;
	}

	bool player::send_player_abilities_packet()
	{
		// todo
		auto player_abilities = packet_builder(0x39)
									.append_i8(0b00000100)
									.append_float(0.2f)
									.append_float(0.2f)
									.build();
		this->_client.send_packet(player_abilities);
		return true;
	}

	bool player::send_move_and_look_packet()
	{
		// todo
		auto player_position_and_look =
			packet_builder(0x08)
				.append_double(this->pos.x)
				.append_double(this->pos.y)
				.append_double(this->pos.z)
				.append_float(this->pos.yaw)
				.append_float(this->pos.pitch)
				.append_bool(false) // todo: check if on ground.
				.build();
		this->_client.send_packet(player_position_and_look);
		return true;
	}

	bool player::send_spawn_player_packet(const player &other)
	{
		// todo
		auto spawn_player =
			packet_builder(0x0c)
				.append_varint(other.id)
				.append_string("776e5cb3-66a5-48b0-9a0f-c29bb888c712")
				.append_string(other._username)
				.append_varint(0)
				.append_i32(other.pos.x * 32)
				.append_i32(other.pos.y * 32)
				.append_i32(other.pos.z * 32)
				.append_i8(0)
				.append_i8(0)
				.append_i16(0)
				.append_bytes(other.get_metadata().get_data())
				.build();

		this->_client.send_packet(spawn_player);
		return true;
	}

	static void compress_buffer(const std::vector<uint8_t> &source,
								std::vector<uint8_t> &dest)
	{
		uint32_t compressed_length = compressBound(source.size());
		uint8_t *dest_buf = new uint8_t[compressed_length];
		compress(dest_buf, (uLongf *)&compressed_length, source.data(),
				 source.size());

		std::vector<uint8_t> data_vector(dest_buf,
										 dest_buf + compressed_length);
		dest.reserve(compressed_length);
		dest.insert(dest.begin(), data_vector.begin(), data_vector.end());
	}

	bool player::send_bulk_chunk_data_packet()
	{
		std::vector<uint8_t> chunk_data;
		std::vector<uint8_t> compressed_chunk_data;

		for (int y = 0; y < 16; y++)
			{
				for (int z = 0; z < 16; z++)
					{
						for (int x = 0; x < 16; x++)
							{
								chunk_data.push_back(y);
							}
					}
			}

		for (int i = 0; i < 16 * 16 * 16; i++)
			{
				chunk_data.push_back(10);
			}
		for (int i = 0; i < 16 * 16 * 16; i++)
			{
				chunk_data.push_back(11);
			}

		for (int i = 0; i < 256; i++)
			{
				chunk_data.push_back(12);
			}

		compress_buffer(chunk_data, compressed_chunk_data);

		auto chunk_data_packet =
			packet_builder(0x21)
				.append_i32(0)					// chunk x
				.append_i32(0)					// chunk z
				.append_bool(true)				// ground-up continuous
				.append_u16(1)					// primary bit map
				.append_u16(0b0000000000000000) // add bit map
				.append_i32(compressed_chunk_data.size())
				.append_bytes(compressed_chunk_data)
				.build();

		this->_client.send_packet(chunk_data_packet);
		return true;
	}
	bool player::send_entity_look(const entity &entity, float yaw, float pitch)
	{
		auto entity_look = packet_builder(0x16)
							   .append_i32(entity.id)
							   .append_i8((int8_t)(yaw * 256.f / 360.f))
							   .append_i8((int8_t)(pitch * 256.f / 360.f))
							   .build();

		this->_client.send_packet(entity_look);
		return true;
	}

	bool player::send_entity_relative_move(const entity &entity, double dx,
										   double dy, double dz)
	{
		int8_t dx_fp = (dx * 32);
		int8_t dy_fp = (dy * 32);
		int8_t dz_fp = (dz * 32);

		auto entity_relative_move = packet_builder(0x15)
										.append_i32(entity.id)
										.append_i8(dx_fp)
										.append_i8(dy_fp)
										.append_i8(dz_fp)
										.build();

		this->_client.send_packet(entity_relative_move);
		return true;
	}
	bool player::send_entity_relative_move_and_look(const entity &entity,
													double dx, double dy,
													double dz, float yaw,
													float pitch)
	{
		int8_t dx_fp = (dx * 32);
		int8_t dy_fp = (dy * 32);
		int8_t dz_fp = (dz * 32);

		auto entity_look_and_relative_move =
			packet_builder(0x17)
				.append_i32(entity.id)
				.append_i8(dx_fp)
				.append_i8(dy_fp)
				.append_i8(dz_fp)
				.append_i8((int8_t)(yaw * 256.f / 360.f))
				.append_i8((int8_t)(pitch * 256.f / 360.f))
				.build();
		this->_client.send_packet(entity_look_and_relative_move);
		return true;
	}

	bool player::send_entity_packet(const entity &entity)
	{
		auto entity_packet = packet_builder(0x14).append_i32(entity.id).build();

		this->_client.send_packet(entity_packet);
		return true;
	}

	bool player::send_entity_head_look(const entity &entity, float yaw)
	{
		auto entity_head_look = packet_builder(0x19)
									.append_i32(entity.id)
									.append_i8((int8_t)(yaw * 256.f / 360.f))
									.build();
		this->_client.send_packet(entity_head_look);
		return true;
	}

	bool player::send_destroy_entities(std::vector<entity *> entities)
	{
		auto destroy_entities = packet_builder(0x13).append_i8(entities.size());
		for (auto entity : entities)
			{
				destroy_entities.append_i32(entity->id);
			}
		this->_client.send_packet(destroy_entities.build());
		return true;
	}

	bool player::send_destroy_entity(const entity &entity)
	{
		auto destroy_entities =
			packet_builder(0x13).append_i8(1).append_i32(entity.id).build();
		this->_client.send_packet(destroy_entities);
		return true;
	}

	bool player::initialize_other_player(const player *other_player)
	{
		this->send_spawn_player_packet(*other_player);
		this->send_entity_head_look(*other_player, other_player->pos.yaw);
		this->send_entity_look(*other_player, other_player->pos.yaw,
							   other_player->pos.pitch);
		return true;
	}
} // namespace wild

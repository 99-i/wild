#pragma once
#include <nlohmann/json.hpp>
#include "common.h"
#include "entity_metadata.h"
#include "living_entity.h"

namespace wild
{
	class client;

	struct player : public living_entity
	{
		//the client associated with this player
		wild::client &_client;
		//the player's username determined by mojang
		std::string _username;

		wild::entity_pos new_move_and_look;

		//todo
		uint64_t uuid[2];

		player(wild::client &client);
		metadata::human_metadata get_metadata() const;

#pragma region Send Packet Methods

		//todo
		//0x01
		//void send_join_game_packet(); /*send info of world player is in*/
		void send_join_game_packet(uint8_t gamemode, int8_t dimension, uint8_t difficulty, uint8_t max_players, std::string level_type); /*send custom*/
		//0x02
		//void send_chat_message_packet(nlohmann::json json);
		//0x03
		//void send_time_update_packet(); /*send the world age and time of day of current player's worl*/
		//void send_time_update_packet(int64_t world_age, int64_t time_of_day); /*send custom*/
		//0x04 TODO
		//void send_entity_equipment(const wild::entity &entity, int16_t slot, wild::slot item);
		//0x05
		void send_spawn_position_packet(); /*send the player's current spawn position*/
		//void send_spawn_position_packet(int32_t x, int32_t y, int32_t z); /*send custom spawn position*/
		//0x06
		//void send_update_health_packet(); /*send player's info*/
		//void send_update_health_packet(float health, int16_t food, float saturation); /*send custom info*/
		//0x07
		//void send_respawn_packet();

		//0x39
		void send_player_abilities_packet();

		//0x08
		void send_move_and_look_packet();

		void send_spawn_player_packet(const wild::player &other);
		void send_bulk_chunk_data_packet();

		void send_entity_relative_look(const wild::entity &entity, float yaw, float pitch);
		void send_entity_relative_move(const wild::entity &entity, float dx, float dy, float dz);
		void send_entity_relative_move_and_look(const wild::entity &entity, float dx, float dy, float dz, float yaw, float pitch);
		void send_entity_packet(const wild::entity &entity);
		void send_entity_head_look(const wild::entity &entity, float yaw);
#pragma endregion
	};
}
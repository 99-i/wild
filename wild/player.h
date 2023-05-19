#pragma once
#include "common.h"
#include "entity.h"
#include "entity_metadata.h"

namespace wild
{
	class client;
	class player : public entity
	{
		//the client associated with this player
		wild::client &_client;
	public:
		//the player's username determined by mojang
		std::string _username;

		//the position that the client thinks the player is in
		//(only updated when the client sends a position packet)
		//useful for correcting the client's position when it differs from server position.
		wild::vec3f client_pos;
		player(wild::client &client);
		wild::client &client() const;
		metadata::human_metadata metadata;

		void send_join_game_packet(uint8_t gamemode, int8_t dimension, uint8_t difficulty, uint8_t max_players, std::string level_type);
		void send_spawn_position_packet();
		void send_player_abilities_packet();
		void send_position_packet();
		void send_spawn_player_packet(const wild::player &other);
	};
}
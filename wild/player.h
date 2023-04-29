#pragma once
#include "common.h"
#include "entity.h"

namespace wild
{
	struct client;
	class player : public entity
	{
		//the player's username determined by mojang
		std::string _username;
		//the client associated with this player
		wild::client &_client;
	public:
		//the position that the client thinks the player is in
		//(only updated when the client sends a position packet)
		//useful for correcting the client's position when it differs from server position.
		wild::vec3f client_pos;
		player(wild::client &client);
		const std::string &username() const;
		const wild::client &client() const;
	};
}
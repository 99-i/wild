#pragma once
#include "common.h"
#include "entity.h"

namespace wild
{
	struct client;
	class player : public entity
	{
		std::string _username;
		wild::client &_client;
	public:
		wild::vec3f client_pos;
		player(wild::client &client);
		const std::string &username() const;
		const wild::client &client() const;
	};
}
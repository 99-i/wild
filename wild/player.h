#pragma once
#include "common.h"
#include "entity.h"

namespace wild
{
	struct client;
	class player : public entity
	{
		std::string username;
		wild::client *client = nullptr;
	};
}
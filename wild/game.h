#pragma once
#include <vector>
#include <optional>
#include "lua_machine.h"

struct w_player;
struct w_server;
struct w_game
{
	std::vector<w_player *> players;
	w_server *server;
	uv_timer_t tick_timer;
	lua_machine lua_vm = lua_machine(this);
	w_game(w_server *server);

	void start();
	void tick();
};
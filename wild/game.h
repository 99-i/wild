#pragma once
#include <asio.hpp>
#include <vector>
#include <optional>
#include <asio/deadline_timer.hpp>
#include "lua_machine.h"

struct w_player;
struct w_server;
struct w_game
{
	std::vector<w_player *> players;
	w_server *server;
	asio::high_resolution_timer tick_timer;
	lua_machine lua_vm = lua_machine(this);
	w_game(w_server *server);

	void start();
	void tick();
};
#include <chrono>
#include "game.h"
#include "server.h"

constexpr int TICK_RATE = 20;

w_game::w_game(w_server *server) : server(server), tick_timer(server->context)
{
}

void w_game::start()
{
	this->lua_vm.start();
	while (true)
	{
		auto begin = std::chrono::high_resolution_clock::now();
		this->tick();
		auto end = std::chrono::high_resolution_clock::now();
		auto time_tick_took = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
		auto time_left = std::chrono::milliseconds(1000 / TICK_RATE) - time_tick_took;

		this->tick_timer.expires_after(time_left);

		this->tick_timer.wait();
	}
}
void w_game::tick()
{
	this->lua_vm.tick();
}
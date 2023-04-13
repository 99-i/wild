#pragma once
#include <sol/sol.hpp>
#include <mutex>

#include "player.h"
#include "packet.h"

struct lua_runnable;

struct runnable_entry
{
	lua_runnable *runnable;
	int32_t ticks_left;
};

//handles everything to do with lua, including plugins.
struct lua_machine
{
private:
	std::mutex state_mutex;
	sol::state *state = nullptr;
	w_game *game;
public:
	lua_machine(w_game *game);
	//resets L to normal.
	void reset();
	//starts the lua machine.
	void start();

	//called when a packet is received by a player.
	void receive_packet(w_client *client, w_packet packet);

	//called when the game ticks.
	void tick();

	void register_runnable(lua_runnable *runnable);
	void stop_runnable(uint32_t id);

	//list of all currently running runnables.
	std::vector<runnable_entry *> runnables;
};

struct lua_runnable
{
	sol::function f;
	lua_machine *machine;
	lua_runnable(lua_machine *machine, sol::function f);
	~lua_runnable();

	enum class run_type
	{
		//run once.
		ONCE,
		//run every time.
		TIMER
	} run_type;

	uint32_t interval;
	uint32_t delay = 0;

	uint32_t id;
	bool stop = false;

	void run_later(uint32_t when);

	void run_timer(uint32_t delay, uint32_t interval);
};

#pragma once
#include <sol/sol.hpp>
#include <mutex>

#include "player.h"
#include "packet.h"

struct w_runnable;

struct runnable_entry
{
	w_runnable *runnable;
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
	void receive_packet(w_client *client, w_packet *packet);

	//called when the game ticks.
	void tick();

	void register_runnable(w_runnable *runnable);
	void stop_runnable(uint32_t id);

	//list of all currently running runnables.
	std::vector<runnable_entry *> runnables;
};

struct w_runnable
{
	sol::function lua_f;
	//void*: any data
	//uint32_t: runnable ID
	typedef std::function<void(void *, uint32_t)> c_function_t;
	c_function_t c_f;
	void *data;
	lua_machine *machine;
	w_runnable(lua_machine *machine, sol::function f);
	w_runnable(lua_machine *machine, c_function_t f, void *data);
	~w_runnable();

	enum class function_type
	{
		LUA_F,
		C_F
	} function_type;

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

	void call();
};

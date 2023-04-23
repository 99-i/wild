#pragma once
#include "common.h"

namespace wild
{
	struct player;
	struct server;

	struct game_update
	{
		enum class type
		{
			PLAYER_JOIN,
			PLAYER_MOVE
		} type;
		union
		{
			struct
			{
				wild::player *player;
			} _join;
		};
	};

	struct runnable_entry
	{
		uint32_t ticks_left;
		uint32_t delay;
		uint32_t interval;
		wild::runnable::run_type run_type;
		uint32_t runnable_id;
		bool stop = false;
		//todo
		enum class function_type_e
		{
			C_FUNCTION,
			//todo
			//LUA_FUNCTION
		} function_type;
		wild::runnable::c_function_t c_f;
		bool call()
		{
			if (this->function_type == function_type_e::C_FUNCTION)
			{
				return this->c_f(this->runnable_id);
			} else
			{
				//todo
			}
			return false;
		}
	};

	struct game
	{
		std::vector<wild::player *> players;
		wild::server *server;
		std::chrono::high_resolution_clock::time_point time_since_last_tick;
		game(wild::server *server);

		std::mutex runnables_mutex;
		//list of all currently running runnables or runnables that are queued to stop.
		std::vector<runnable_entry *> runnables;

		//returns the id of the runnable (to pass to stop_runnable)
		uint32_t create_c_runnable(wild::runnable::c_function_t f, wild::runnable::run_settings_t settings);
		//todo
		//uint32_t create_lua_runnable();
		void stop_runnable(uint32_t id);

		void start();
		void tick();
	};
}

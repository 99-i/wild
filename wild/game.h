#pragma once
#include <queue>
#include <chrono>
#include "common.h"

namespace wild
{
	struct player;
	struct server;

	struct game_event
	{
		struct player_join_event
		{
			wild::player *player;
		};
		struct player_move_event
		{
			wild::player *player;
			vec3f new_pos;
		};

		enum class type
		{
			PLAYER_JOIN,
			PLAYER_MOVE
		} type;
		union
		{
			player_join_event _player_join;
			player_move_event _player_move;
		};

		static game_event player_join(wild::player *player)
		{
			game_event event;
			event.type = type::PLAYER_JOIN;
			event._player_join.player = player;
			return event;
		}

	private:
		game_event()
		{
		}
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

	class game
	{
		std::vector<wild::player *> players;
		wild::server &server;
		std::chrono::high_resolution_clock::time_point time_since_last_tick;

		wild::counter<uint32_t> runnable_id_counter;
		std::mutex runnables_mutex;
		//list of all currently running runnables or runnables that are queued to stop.
		std::vector<runnable_entry *> runnables;

		std::mutex pending_events_mutex;
		std::queue<game_event> pending_events;

		void handle_event(game_event event);
#pragma region UpdateHandlers
		void handle_player_join_event(game_event::player_join_event event);
		void handle_player_move_event(game_event::player_move_event event);
#pragma endregion

	public:
		game(wild::server &server);
		void queue_event(game_event event);

		//returns the id of the runnable (to pass to stop_runnable)
		uint32_t create_c_runnable(wild::runnable::c_function_t f, wild::runnable::run_settings_t settings);
		//todo
		//uint32_t create_lua_runnable();
		void stop_runnable(uint32_t id);

		void start();
		void tick();
	};
}

#pragma once
#include "../common.hpp"
#include "../runnable.hpp"
#include "entity/entity.hpp"
#include "world/world.hpp"
#include <asio.hpp>
#include <asio/deadline_timer.hpp>
#include <queue>

namespace wild
{
	struct player;
	struct server;

	// an event that the game receives and handles in its main loop.
	struct game_event
	{
		// when a player has joined. the game handles actually spawning in the
		// player and sending the packets.
		struct player_join_event
		{
			wild::player *player;
		};
		// when a player leaves.
		struct player_leave_event
		{
			wild::player *player;
		};
		// when a player sends only a move packet (only x,y,z change)
		struct player_move_event
		{
			wild::player *player;
			wild::vec3d new_pos;
		};
		// when a player sends only a look packet (only yaw, pitch change)
		struct player_look_event
		{
			wild::player *player;
			float new_yaw;
			float new_pitch;
		};
		// when a player sends a move and look packet (everything changes.)
		struct player_move_and_look_event
		{
			wild::player *player;
			wild::entity_pos new_pos;
		};

		enum class event_type
		{
			PLAYER_JOIN,
			PLAYER_LEAVE,
			PLAYER_MOVE,
			PLAYER_LOOK,
			PLAYER_MOVE_AND_LOOK
		} type;
		union
		{
			player_join_event _player_join;
			player_leave_event _player_leave;
			player_move_event _player_move;
			player_look_event _player_look;
			player_move_and_look_event _player_move_and_look;
		};

		static inline game_event player_join(wild::player *player)
		{
			game_event event;
			event.type = event_type::PLAYER_JOIN;
			event._player_join.player = player;
			return event;
		}

		static inline game_event player_leave(wild::player *player)
		{
			game_event event;
			event.type = event_type::PLAYER_LEAVE;
			event._player_leave.player = player;
			return event;
		}
		static inline game_event player_move(wild::player *player,
											 wild::vec3d new_pos)
		{
			game_event event;
			event.type = event_type::PLAYER_MOVE;
			event._player_move.new_pos = new_pos;
			event._player_move.player = player;
			return event;
		}

		static inline game_event player_look(wild::player *player,
											 float new_yaw, float new_pitch)
		{
			game_event event;
			event.type = event_type::PLAYER_LOOK;
			event._player_look.new_yaw = new_yaw;
			event._player_look.new_pitch = new_pitch;
			event._player_look.player = player;
			return event;
		}

		static inline game_event player_move_and_look(wild::player *player,
													  wild::entity_pos new_pos)
		{
			game_event event;
			event.type = event_type::PLAYER_MOVE_AND_LOOK;
			event._player_move_and_look.player = player;
			event._player_move_and_look.new_pos = new_pos;
			return event;
		}

	  private:
		game_event() {}
	};

	// a runnable that gets ran in a certain amount of ticks or on a timer
	struct runnable_entry
	{
		// the amount of ticks left before this runnable gets called.
		uint32_t ticks_left;
		// when first starting the runnable, this determines when the first time
		// the runnable is called is
		uint32_t delay;
		// if the runnable is a timer, the interval at which it runs.
		uint32_t interval;
		wild::runnable::run_type run_type;
		// every runnable has a runnable id that is used to stop it.
		uint32_t runnable_id;
		// if, on the next time this runnable should run, it should instead be
		// destroyed
		bool stop = false;
		enum class function_type_e
		{
			// the function is a c or c++ function, not a lua one (which is
			// created in plugins).
			C_FUNCTION,
			// todo
			// LUA_FUNCTION
		} function_type;
		// an std::function of the function to run if it is a C_FUNCTION
		wild::runnable::c_function_t c_f;
		// call the function.
		bool call()
		{
			if (this->function_type == function_type_e::C_FUNCTION)
				{
					return this->c_f(this->runnable_id);
				}
			else
				{
					// todo
				}
			return false;
		}
	};

	// the main game server. one game per server.
	class game
	{
		bool running = false;
		wild::server &server;

		asio::io_context game_context;
		asio::system_timer tick_timer;
		void restart_timer(std::chrono::system_clock::duration after);
		// a counter that determines which runnable id to assign to the next
		// runnable
		wild::counter<uint32_t> runnable_id_counter;
		// a mutex to runnables.
		std::mutex runnables_mutex;
		// list of all currently running runnables or runnables that are queued
		// to stop.
		std::vector<runnable_entry *> runnables;

		void tick_runnables();

		// a mutex to pending_events.
		std::mutex pending_events_mutex;
		// a queue of all the game events that have yet to be processed by the
		// game.
		std::queue<game_event> pending_events;

		// internally handle the event.
		void handle_event(game_event event);

#pragma region UpdateHandlers
		void handle_player_join_event(game_event::player_join_event event);
		void handle_player_leave_event(game_event::player_leave_event event);
		void handle_player_move_event(game_event::player_move_event event);
		void handle_player_look_event(game_event::player_look_event event);
		void handle_player_move_and_look_event(
			game_event::player_move_and_look_event event);
#pragma endregion

		uint64_t current_tick_number = 0;

	  public:
		// todo
		wild::world *overworld;

		std::vector<wild::player *> get_players();
		game(wild::server &server);
		// queue an event to be handled on the next iteration of the inner game
		// loop
		//(NOT THE TICK LOOP)
		// to make something happen on the next tick, call create_c_runnable
		// with a delay of 0.
		void queue_event(game_event event);

		// returns the id of the runnable (to pass to stop_runnable)
		// automatically starts the runnable.
		uint32_t create_c_runnable(wild::runnable::c_function_t f,
								   wild::runnable::run_settings_t settings);
		// todo
		// uint32_t create_lua_runnable();
		// set the stop flag of a runnable to true, essentially destroying it.
		void stop_runnable(uint32_t id);

		void start();
		void stop();
		void tick();
	};
} // namespace wild

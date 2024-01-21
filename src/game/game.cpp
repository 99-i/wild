#include "game/game.hpp"
#include "common.hpp"
#include "game/entity/player.hpp"
#include "net/client.hpp"
#include "net/server.hpp"
#include <chrono>
#include <plog/Log.h>

namespace wild
{
	constexpr int TICK_RATE_MS = 50;
	using namespace std::literals;

	game::game(wild::server &server) : server(server), tick_timer(game_context)
	{
		this->overworld = new world();
		this->restart_timer(std::chrono::milliseconds(50));
	}

	void game::queue_event(game_event event)
	{
		std::lock_guard<std::mutex> pending_events_guard{
			this->pending_events_mutex};
		this->pending_events.push(event);
	}

	void game::handle_event(game_event event)
	{
		switch (event.type)
			{
			case game_event::event_type::PLAYER_JOIN:
				return handle_player_join_event(event._player_join);
			case game_event::event_type::PLAYER_LEAVE:
				return handle_player_leave_event(event._player_leave);
			case game_event::event_type::PLAYER_MOVE:
				return handle_player_move_event(event._player_move);
			case game_event::event_type::PLAYER_LOOK:
				return handle_player_look_event(event._player_look);
			case game_event::event_type::PLAYER_MOVE_AND_LOOK:
				return handle_player_move_and_look_event(
					event._player_move_and_look);
			}
	}

	// player is already created and player <-> client are linked by the time
	// this event comes in.
	void game::handle_player_join_event(game_event::player_join_event event)
	{
		this->overworld->spawn_entity(event.player);
	}

	void game::handle_player_leave_event(game_event::player_leave_event event)
	{
		event.player->remove();
	}

	void game::handle_player_move_event(game_event::player_move_event event)
	{
		event.player->move(event.new_pos.x, event.new_pos.y, event.new_pos.z);
	}

	void game::handle_player_look_event(game_event::player_look_event event)
	{
		event.player->pos.pitch = event.new_pitch;
		event.player->pos.yaw = event.new_yaw;
		event.player->pos.changed_look = true;
	}

	void game::handle_player_move_and_look_event(
		game_event::player_move_and_look_event event)
	{
		event.player->pos.pitch = event.new_pos.pitch;
		event.player->pos.yaw = event.new_pos.yaw;
		event.player->pos.changed_look = true;
		event.player->move(event.new_pos.x, event.new_pos.y, event.new_pos.z);
	}

	void game::stop()
	{
		PLOGD << "Game exiting...";
		this->running = false;
		this->game_context.stop();
		// todo: do end routine stuff.
	}

	void game::restart_timer(std::chrono::system_clock::duration after)
	{
		this->tick_timer.expires_after(after);

		this->tick_timer.async_wait(std::bind(&game::tick, this));
	}

	// todo: message queue, non-busy wait
	void game::start()
	{
		this->running = true;
		this->game_context.run();
	}

#define STOP_RUNNABLE                                                          \
	delete entry;                                                              \
	this->runnables.erase(this->runnables.begin() + i);                        \
	i--;

	void game::tick_runnables()
	{
		for (size_t i = 0; i < this->runnables.size(); i++)
			{
				runnable_entry *entry = this->runnables[i];
				if (entry->stop)
					{
						std::lock_guard<std::mutex> runnables_guard{
							this->runnables_mutex};
						STOP_RUNNABLE;
						continue;
					}
				if (entry->ticks_left <= 0)
					{
						// todo
						bool continue_runnable = entry->call();
						if (!continue_runnable)
							{
								std::lock_guard<std::mutex> runnables_guard{
									this->runnables_mutex};
								STOP_RUNNABLE;
								continue;
							}
						if (entry->run_type == runnable::run_type::TIMER)
							{
								entry->ticks_left = entry->interval;
							}
						else
							{
								std::lock_guard<std::mutex> runnables_guard{
									this->runnables_mutex};
								STOP_RUNNABLE;
							}
					}
				entry->ticks_left--;
			}
	}
#undef STOP_RUNNABLE

	void game::tick()
	{
		auto begin = std::chrono::system_clock::now();
		{
			PLOGD << "Tick!   " << this->current_tick_number << "       "
				  << std::chrono::time_point_cast<std::chrono::seconds>(
						 std::chrono::system_clock::now());
			std::queue<game_event> pending_events_copy;

			this->pending_events_mutex.lock();
			this->pending_events.swap(pending_events_copy);
			this->pending_events_mutex.unlock();

			while (!pending_events_copy.empty())
				{
					this->handle_event(pending_events_copy.front());
					pending_events_copy.pop();
				}

			this->tick_runnables();
			this->overworld->tick();
			// todo
			// this->tick_physics();
			// this->tick_chunks();
			this->current_tick_number++;
		}
		auto end = std::chrono::system_clock::now();
		auto duration = end - begin;
		if (duration > std::chrono::milliseconds(50))
			{
				// todo better log msg
				PLOGD << "Tick took way too long!";
				this->restart_timer(std::chrono::milliseconds(0));
			}
		else
			{
				this->restart_timer(std::chrono::milliseconds(50) - duration);
			}
	}

	void game::stop_runnable(uint32_t id)
	{
		std::lock_guard<std::mutex> runnables_guard{this->runnables_mutex};
		for (auto it = this->runnables.begin(); it != this->runnables.end();
			 it++)
			{
				if ((*it)->runnable_id == id)
					{
						(*it)->stop = true;
						break;
					}
			}
	}

	uint32_t game::create_c_runnable(runnable::c_function_t f,
									 runnable::run_settings_t settings)
	{
		std::lock_guard<std::mutex> runnables_guard{this->runnables_mutex};
		runnable_entry *entry = new runnable_entry;
		entry->delay = std::get<1>(settings);
		entry->run_type = std::get<0>(settings);
		if (entry->run_type == runnable::run_type::TIMER)
			{
				entry->interval = std::get<2>(settings);
			}
		entry->ticks_left = entry->delay;
		entry->c_f = f;
		entry->function_type = runnable_entry::function_type_e::C_FUNCTION;
		entry->runnable_id = this->runnable_id_counter.next();

		this->runnables.push_back(entry);

		return entry->runnable_id;
	}
} // namespace wild

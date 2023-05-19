#include <chrono>
#include <plog/Log.h>
#include "server.h"
#include "common.h"
#include "clientbound_packet.h"
#include "game.h"
#include "player.h"
#include "client.h"

constexpr int TICK_RATE_MS = 50;
using namespace std::literals;

wild::game::game(wild::server &server) : server(server)
{
}

void wild::game::queue_event(game_event event)
{
	this->pending_events_mutex.lock();
	this->pending_events.push(event);
	this->pending_events_mutex.unlock();
}

void wild::game::handle_event(game_event event)
{
	switch (event.type)
	{
		case game_event::type::PLAYER_JOIN:
			return handle_player_join_event(event._player_join);
		case game_event::type::PLAYER_MOVE:
			return handle_player_move_event(event._player_move);
		case game_event::type::PLAYER_LEAVE:
			return handle_player_leave_event(event._player_leave);
	}
}

//player is already created  and player <-> client are linked by the time this event comes in.
void wild::game::handle_player_join_event(game_event::player_join_event event)
{
	this->players_mutex.lock();
	//todo: spawn player in

	event.player->send_join_game_packet(1, 0, 0, 100, "default");
	event.player->send_spawn_position_packet();
	event.player->send_player_abilities_packet();
	event.player->send_position_packet();

	for (auto &player : this->players)
	{
		player->send_spawn_player_packet(*event.player);
		event.player->send_spawn_player_packet(*player);
	}

	this->players.push_back(event.player);

	this->players_mutex.unlock();
}

void wild::game::handle_player_leave_event(game_event::player_leave_event event)
{
	this->players_mutex.lock();

	auto index = std::find(this->players.begin(), this->players.end(), event.player);
	if (index != this->players.end())
		this->players.erase(index);

	this->players_mutex.unlock();
}

void wild::game::handle_player_move_event(game_event::player_move_event event)
{
}

//todo: message queue, non-busy wait
void wild::game::start()
{
	while (true)
	{
		this->pending_events_mutex.lock();
		while (!this->pending_events.empty())
		{
			this->handle_event(this->pending_events.front());
			this->pending_events.pop();
		}
		this->pending_events_mutex.unlock();

		auto now = std::chrono::high_resolution_clock::now();
		if ((now - this->time_since_last_tick) > 50ms)
		{
			this->tick();
			this->time_since_last_tick = std::chrono::high_resolution_clock::now();
		}
	}
}
#define STOP_RUNNABLE it = this->runnables.erase(it);\
				delete entry;\
				if (it >= this->runnables.end()) {break;}
void wild::game::tick()
{
	this->runnables_mutex.lock();
	for (auto it = this->runnables.begin(); it != this->runnables.end(); it++)
	{
		runnable_entry *entry = *it;
		if (entry->stop)
		{
			STOP_RUNNABLE;
			continue;
		}
		if (entry->ticks_left <= 0)
		{
			//todo
			bool continue_runnable = entry->call();
			if (!continue_runnable)
			{
				STOP_RUNNABLE;
				continue;
			}
			if (entry->run_type == wild::runnable::run_type::TIMER)
			{
				entry->ticks_left = entry->interval;
			} else
			{
				STOP_RUNNABLE;
			}
		}
		entry->ticks_left--;
	}
	this->runnables_mutex.unlock();

	//todo
	//this->tick_physics();
	//this->tick_chunks();
}
#undef STOP_RUNNABLE

void wild::game::stop_runnable(uint32_t id)
{
	this->runnables_mutex.lock();
	for (auto it = this->runnables.begin(); it != this->runnables.end(); it++)
	{
		if ((*it)->runnable_id == id)
		{
			(*it)->stop = true;
			break;
		}
	}
	this->runnables_mutex.unlock();
}

uint32_t wild::game::create_c_runnable(wild::runnable::c_function_t f, wild::runnable::run_settings_t settings)
{
	runnable_entry *entry = new runnable_entry;
	entry->delay = std::get<1>(settings);
	entry->run_type = std::get<0>(settings);
	if (entry->run_type == wild::runnable::run_type::TIMER)
	{
		entry->interval = std::get<2>(settings);
	}
	entry->ticks_left = entry->delay;
	entry->c_f = f;
	entry->function_type = runnable_entry::function_type_e::C_FUNCTION;
	entry->runnable_id = this->runnable_id_counter.next();

	this->runnables_mutex.lock();
	PLOGD << "Starting runnable " << entry->runnable_id;
	this->runnables.push_back(entry);
	this->runnables_mutex.unlock();

	return entry->runnable_id;
}
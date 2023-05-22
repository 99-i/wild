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
		case game_event::event_type::PLAYER_JOIN:
			return handle_player_join_event(event._player_join);
		case game_event::event_type::PLAYER_LEAVE:
			return handle_player_leave_event(event._player_leave);
		case game_event::event_type::PLAYER_MOVE:
			return handle_player_move_event(event._player_move);
		case game_event::event_type::PLAYER_LOOK:
			return handle_player_look_event(event._player_look);
		case game_event::event_type::PLAYER_MOVE_AND_LOOK:
			return handle_player_move_and_look_event(event._player_move_and_look);
	}
}

//player is already created  and player <-> client are linked by the time this event comes in.
void wild::game::handle_player_join_event(game_event::player_join_event event)
{
	std::lock_guard<std::mutex> players_lock{ this->players_mutex };
	//todo: spawn player in
	event.player->pos.x = 9;
	event.player->pos.y = 17;
	event.player->pos.z = 9;

	event.player->send_join_game_packet(1, 0, 0, 100, "default");
	event.player->send_spawn_position_packet();
	event.player->send_player_abilities_packet();
	event.player->send_bulk_chunk_data_packet();
	event.player->send_move_and_look_packet();

	for (auto &player : this->players)
	{
		player->send_spawn_player_packet(*event.player);
		player->send_entity_head_look(*event.player, event.player->pos.yaw);
		event.player->send_spawn_player_packet(*player);
		event.player->send_entity_head_look(*player, player->pos.yaw);
	}

	this->players.push_back(event.player);
}

void wild::game::handle_player_leave_event(game_event::player_leave_event event)
{
	std::lock_guard<std::mutex> players_guard{ this->players_mutex };

	auto index = std::find(this->players.begin(), this->players.end(), event.player);
	if (index != this->players.end())
		this->players.erase(index);

	//todo: send disconnect packet, tell other players they left etc.
}

void wild::game::handle_player_move_event(game_event::player_move_event event)
{
	this->position_changed_map[event.player] = wild::game_event::event_type::PLAYER_MOVE;
	event.player->new_move_and_look.x = event.new_pos.x;
	event.player->new_move_and_look.y = event.new_pos.y;
	event.player->new_move_and_look.z = event.new_pos.z;
}

void wild::game::handle_player_look_event(game_event::player_look_event event)
{
	if (event.player->pos.pitch == event.new_pitch && event.player->pos.yaw == event.new_yaw)
		return;
	this->position_changed_map[event.player] = wild::game_event::event_type::PLAYER_LOOK;
	event.player->new_move_and_look.yaw = event.new_yaw;
	event.player->new_move_and_look.pitch = event.new_pitch;
}
void wild::game::handle_player_move_and_look_event(game_event::player_move_and_look_event event)
{
	this->position_changed_map[event.player] = wild::game_event::event_type::PLAYER_MOVE_AND_LOOK;
	event.player->new_move_and_look = event.new_pos;
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

//todo: players far away shouldn't get that packet.
void wild::game::tick_player_positions()
{
	std::lock_guard<std::mutex> players_guard{ this->players_mutex };
	for (auto it = this->players.begin(); it != this->players.end(); it++)
	{
		auto player = *it;
		if (this->position_changed_map.contains(player))
		{
			auto type = this->position_changed_map[player];
			switch (type)
			{
				case game_event::event_type::PLAYER_LOOK:
				{
					float new_yaw = player->new_move_and_look.yaw;
					float new_pitch = player->new_move_and_look.pitch;
					for (int i = 0; i < this->players.size(); i++)
					{
						if (this->players[i] != player)
						{
							this->players[i]->send_entity_relative_look(*player, new_yaw, new_pitch);
							this->players[i]->send_entity_head_look(*player, new_yaw);
						}
					}
					player->pos.yaw = new_yaw;
					player->pos.pitch = new_pitch;
				}
				break;
				case game_event::event_type::PLAYER_MOVE:
				{
					float dx = player->new_move_and_look.x - player->pos.x;
					float dy = player->new_move_and_look.y - player->pos.y;
					float dz = player->new_move_and_look.z - player->pos.z;
					for (int i = 0; i < this->players.size(); i++)
					{
						if (this->players[i] != player)
						{
							this->players[i]->send_entity_relative_move(*player, dx, dy, dz);
						}
					}
					player->pos.x = player->new_move_and_look.x;
					player->pos.y = player->new_move_and_look.y;
					player->pos.z = player->new_move_and_look.z;
				}
				break;
				case game_event::event_type::PLAYER_MOVE_AND_LOOK:
				{
					float yaw = player->new_move_and_look.yaw;
					float pitch = player->new_move_and_look.pitch;
					float dx = player->new_move_and_look.x - player->pos.x;
					float dy = player->new_move_and_look.y - player->pos.y;
					float dz = player->new_move_and_look.z - player->pos.z;

					for (int i = 0; i < this->players.size(); i++)
					{
						if (this->players[i] != player)
						{
							this->players[i]->send_entity_relative_move_and_look(*player, dx, dy, dz, yaw, pitch);
							this->players[i]->send_entity_head_look(*player, yaw);
						}
					}

					player->pos = player->new_move_and_look;
				}
				break;
			}
		} else
		{
			//for (int i = 0; i < this->players.size(); i++)
			//{
			//	if (this->players[i] != player)
			//	{
			//		this->players[i]->send_entity_packet(*player);
			//	}
			//}
		}
	}
}

#define STOP_RUNNABLE delete entry;\
			this->runnables.erase(this->runnables.begin() + i);\
			i--;
void wild::game::tick_runnables()
{
	for (size_t i = 0; i < this->runnables.size(); i++)
	{
		runnable_entry *entry = this->runnables[i];
		if (entry->stop)
		{
			std::lock_guard<std::mutex> runnables_guard{ this->runnables_mutex };
			STOP_RUNNABLE;
			continue;
		}
		if (entry->ticks_left <= 0)
		{
			//todo
			bool continue_runnable = entry->call();
			if (!continue_runnable)
			{
				std::lock_guard<std::mutex> runnables_guard{ this->runnables_mutex };
				STOP_RUNNABLE;
				continue;
			}
			if (entry->run_type == wild::runnable::run_type::TIMER)
			{
				entry->ticks_left = entry->interval;
			} else
			{
				std::lock_guard<std::mutex> runnables_guard{ this->runnables_mutex };
				STOP_RUNNABLE;
			}
		}
		entry->ticks_left--;
	}
}
#undef STOP_RUNNABLE

void wild::game::tick()
{
	this->tick_runnables();
	this->tick_player_positions();
	//todo
	//this->tick_physics();
	//this->tick_chunks();
}

void wild::game::stop_runnable(uint32_t id)
{
	std::lock_guard<std::mutex> runnables_guard{ this->runnables_mutex };
	for (auto it = this->runnables.begin(); it != this->runnables.end(); it++)
	{
		if ((*it)->runnable_id == id)
		{
			(*it)->stop = true;
			break;
		}
	}
}

uint32_t wild::game::create_c_runnable(wild::runnable::c_function_t f, wild::runnable::run_settings_t settings)
{
	std::lock_guard<std::mutex> runnables_guard{ this->runnables_mutex };
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

	PLOGD << "Starting runnable " << entry->runnable_id;
	this->runnables.push_back(entry);

	return entry->runnable_id;
}
#include <filesystem>
#include <iostream>
#include <zlib.h>
#include <plog/Log.h>
#include "random.h"
#include "console.h"
#include "server.h"
#include "game.h"
#include "lua_machine.h"
#include "lfs.h"
#include "clientbound_packet.h"

lua_machine::lua_machine(w_game *game) : game(game)
{
	this->reset();
}

void lua_machine::reset()
{
	PLOGI << "Resetting VM";
	this->state_mutex.lock();
	PLOGI << "Clearing runnable entries table";
	for (auto &entry : this->runnables)
	{
		delete entry->runnable;
		delete entry;
	}
	this->runnables.clear();

	//todo

	this->state_mutex.unlock();
}

//starts the lua machine.
void lua_machine::start()
{
	PLOGI << "Starting VM";
	this->state_mutex.lock();
	//todo
	this->state_mutex.unlock();
}

void lua_machine::receive_packet(w_client *client, w_packet *packet)
{
	this->state_mutex.lock();
	//todo
	this->state_mutex.unlock();
}

void lua_machine::tick()
{
	this->state_mutex.lock();
	for (auto it = this->runnables.begin(); it != this->runnables.end(); it++)
	{
		runnable_entry *entry = *it;
		if (entry->runnable->stop)
		{
			it = this->runnables.erase(it);
			delete entry->runnable;
			delete entry;
			if (it >= this->runnables.end())
				break;

			continue;
		}
		entry->ticks_left--;
		if (entry->ticks_left <= 0)
		{
			//todo
			//entry->runnable->call();
			if (entry->runnable->run_type == w_runnable::run_type::TIMER)
			{
				entry->ticks_left = entry->runnable->interval;
			} else
			{
				it = this->runnables.erase(it);
				delete entry->runnable;
				delete entry;
				if (it >= this->runnables.end())
					break;
			}
		}
	}
	this->state_mutex.unlock();
}

void lua_machine::register_runnable(w_runnable *runnable)
{
	runnable_entry *entry = new runnable_entry();
	entry->runnable = runnable;
	entry->ticks_left = runnable->delay;
	this->runnables.push_back(entry);
}
void lua_machine::stop_runnable(uint32_t id)
{
	for (auto it = this->runnables.begin(); it != this->runnables.end(); it++)
	{
		if ((*it)->runnable->id == id)
		{
			(*it)->runnable->stop = true;
			break;
		}
	}
}

w_runnable::w_runnable(lua_machine *machine, c_function_t f) : machine(machine), c_f(f)
{
	this->id = Random::random_u32();
	this->function_type = function_type::C_F;
}

w_runnable::~w_runnable()
{
}

void w_runnable::run_later(uint32_t when)
{
	this->run_type = run_type::ONCE;
	this->delay = when;
	this->machine->register_runnable(this);
}

void w_runnable::run_timer(uint32_t delay, uint32_t interval)
{
	this->run_type = run_type::TIMER;
	this->delay = delay;
	this->interval = interval;
	this->machine->register_runnable(this);
}

void w_runnable::call()
{
	if (this->function_type == function_type::LUA_F)
	{
		//todo
		//this->lua_f(this->id);
	} else
	{
		this->c_f(this->id);
	}
}
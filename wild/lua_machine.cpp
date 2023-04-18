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
	PLOGD << "Resetting Lua VM";
	this->state_mutex.lock();
	PLOGD << "Clearing runnables table";
	for (auto &entry : this->runnables)
	{
		delete entry->runnable;
		delete entry;
	}
	this->runnables.clear();

	PLOGD << "Deleting old state";
	if (this->state != nullptr)
		delete this->state;
	PLOGD << "Constructing VM";
	this->state = new sol::state();
	this->state_mutex.unlock();
}

//starts the lua machine.
void lua_machine::start()
{
	this->state_mutex.lock();
	this->state->open_libraries(sol::lib::base, sol::lib::io,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::debug,
		sol::lib::bit32,
		sol::lib::ffi);

	this->state->require("lfs", luaopen_lfs, true);

	this->state->set_function("print_", [](char const *caller, char const *str)
		{
			c.reset_line();
			std::cout << "[LUA-" << caller << "] " << str << '\n';
			c.place_line();
		});
	this->state->set_function("destroy_runnable", [&](uint32_t id)
		{
			this->stop_runnable(id);
			return true;
		});

	{
		sol::usertype<w_runnable> runnable_usertype = this->state->new_usertype<w_runnable>("Runnable");
		runnable_usertype.set_function("new", [&](sol::object o1, sol::object o2)
			{
				if (o2.get_type() == sol::type::function)
				{
					return new w_runnable(this, o2);
				}
			});
		runnable_usertype.set_function("run_later", &w_runnable::run_later);
		runnable_usertype.set_function("run_timer", &w_runnable::run_timer);
	}
	this->state_mutex.unlock();
}

void lua_machine::receive_packet(w_client *client, w_packet *packet)
{
	this->state_mutex.lock();
	//sol::function get_packet_lua = (*this->state)["GetPacket"];

	//get_packet_lua(client, &packet);
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
			entry->runnable->call();
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

w_runnable::w_runnable(lua_machine *machine, sol::function f) : machine(machine), lua_f(f)
{
	this->id = Random::random_u32(0, UINT32_MAX);
	this->function_type = function_type::LUA_F;
}

w_runnable::w_runnable(lua_machine *machine, c_function_t f, void *data) : machine(machine), c_f(f), data(data)
{
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<uint32_t> uni(0, INT_MAX);

	this->id = uni(rng);
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
		this->lua_f(this->id);
	else
		this->c_f(this->data, this->id);
}
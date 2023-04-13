#include <filesystem>
#include <iostream>
#include <zlib.h>
#include <random>
#include <plog/Log.h>
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
		sol::usertype<w_packet> packet_usertype = this->state->new_usertype<w_packet>("Packet");

		packet_usertype.set("name", sol::readonly(&w_packet::name));
		packet_usertype.set_function("get_field", [&](w_packet *p, const char *arg) -> sol::object
			{
				if (p->data.contains(arg))
				{
					return sol::make_object(*this->state, p->data[arg]);
				} else
				{
					return sol::nil;
				}
			});
		packet_usertype["id"] = sol::property([](w_packet *packet)
			{
				return packet->form->id;
			});
	}
	{
		sol::usertype<lua_clientbound_packet> cb_packet_usertype = this->state->new_usertype<lua_clientbound_packet>("ClientboundPacket", sol::constructors<lua_clientbound_packet(int)>());
		cb_packet_usertype["id"] = &lua_clientbound_packet::id;
		cb_packet_usertype["write_bool"] = &lua_clientbound_packet::write_bool;
		cb_packet_usertype["write_i8"] = &lua_clientbound_packet::write_i8;
		cb_packet_usertype["write_u8"] = &lua_clientbound_packet::write_u8;
		cb_packet_usertype["write_i16"] = &lua_clientbound_packet::write_i16;
		cb_packet_usertype["write_u16"] = &lua_clientbound_packet::write_u16;
		cb_packet_usertype["write_i32"] = &lua_clientbound_packet::write_i32;
		cb_packet_usertype["write_i64"] = &lua_clientbound_packet::write_i64;
		cb_packet_usertype["write_float"] = &lua_clientbound_packet::write_float;
		cb_packet_usertype["write_double"] = &lua_clientbound_packet::write_double;
		cb_packet_usertype["write_string"] = &lua_clientbound_packet::write_string;
		cb_packet_usertype["write_varint"] = &lua_clientbound_packet::write_varint;
	}
	{
		sol::usertype<w_client> client_usertype = this->state->new_usertype<w_client>("Client");

		client_usertype["state"] = sol::property(&w_client::get_state_str, &w_client::set_state_str);
		client_usertype.set("player", sol::readonly(&w_client::player));
		client_usertype.set_function("send_packet", &w_client::send_packet);
		client_usertype.set_function("is_disconnected", [&](w_client *client)
			{
				return client->disconnect;
			});
		client_usertype.set_function("disconnect", &w_client::disconnect_client);
		client_usertype.set_function(sol::meta_function::new_index, [](w_client *client, std::string key, sol::object value)
			{
				client->lua_obj_data[key] = value;
			});
		client_usertype.set_function(sol::meta_function::index, [](w_client *client, std::string key)
			{
				return client->lua_obj_data[key];
			});
	}
	{
		sol::usertype<w_player> player_usertype = this->state->new_usertype<w_player>("Player");
		player_usertype["username"] = &w_player::username;
	}
	{
		sol::usertype<lua_runnable> runnable_usertype = this->state->new_usertype<lua_runnable>("Runnable");
		runnable_usertype.set_function("new", [&](sol::object o1, sol::object o2)
			{
				if (o2.get_type() == sol::type::function)
				{
					return new lua_runnable(this, o2);
				}
			});
		runnable_usertype.set_function("run_later", &lua_runnable::run_later);
		runnable_usertype.set_function("run_timer", &lua_runnable::run_timer);
	}
	{
		this->state->set_function("compress", [](std::vector<uint8_t> data)
			{
				uLong compressed_size = compressBound(data.size());
				uint8_t *compressed = new uint8_t[compressed_size];
				compress(compressed, &compressed_size, data.data(), data.size());
				auto vec = std::vector<uint8_t>(compressed, compressed + compressed_size);
				delete[] compressed;
				return vec;
			});
	}
	std::ostringstream buffer;
	buffer << std::filesystem::current_path().string() << "/code/main.lua";
	std::string main_file = buffer.str();
	this->state->do_file(main_file.c_str());
	this->state_mutex.unlock();
}

void lua_machine::receive_packet(w_client *client, w_packet packet)
{
	this->state_mutex.lock();
	sol::function get_packet_lua = (*this->state)["GetPacket"];

	get_packet_lua(client, &packet);
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
			if (it >= this->runnables.end())
				break;

			continue;
		}
		entry->ticks_left--;
		if (entry->ticks_left <= 0)
		{
			entry->runnable->f(entry->runnable->id);
			if (entry->runnable->run_type == lua_runnable::run_type::TIMER)
			{
				entry->ticks_left = entry->runnable->interval;
			} else
			{
				it = this->runnables.erase(it);
				if (it >= this->runnables.end())
					break;
			}
		}
	}
	this->state_mutex.unlock();
}

void lua_machine::register_runnable(lua_runnable *runnable)
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

lua_runnable::lua_runnable(lua_machine *machine, sol::function f) : machine(machine), f(f)
{
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<uint32_t> uni(0, INT_MAX);

	this->id = uni(rng);
}

lua_runnable::~lua_runnable()
{
}

void runnable_cb(uv_timer_t *timer)
{
	lua_runnable *runnable = reinterpret_cast<lua_runnable *>(timer);
	runnable->f();
}

void lua_runnable::run_later(uint32_t when)
{
	this->run_type = run_type::ONCE;
	this->delay = when;
	this->machine->register_runnable(this);
}

void lua_runnable::run_timer(uint32_t delay, uint32_t interval)
{
	this->run_type = run_type::TIMER;
	this->delay = delay;
	this->interval = interval;
	this->machine->register_runnable(this);
}
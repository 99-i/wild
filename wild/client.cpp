#include <plog/Log.h>
#include <map>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include "client.h"
#include "clientbound_packet.h"
#include "common.h"
#include "packet.h"
#include "player.h"
#include "server.h"

typedef void(wild::client:: *packet_callback)(const wild::packet &packet);

wild::client::client(asio::ip::tcp::socket &&socket, wild::server &server) : server(server), socket(std::move(socket))
{
}

bool wild::client::handle_data(const std::vector<uint8_t> &data)
{
	for (uint8_t byte : data)
	{
		switch (this->read_state)
		{
			//this is a byte of the length varint.
			case READ_LENGTH_VARINT:
			{
				if (this->packet_length_reader.push_byte(byte))
				{
					this->read_state = WAITING_FOR_ALL_DATA;
					this->packet_length_remaining = this->packet_length_reader.value;
				}
				break;
			}
			case WAITING_FOR_ALL_DATA:
			{
				this->packet_length_remaining--;
				this->packet_buffer.push_back(byte);

				if (this->packet_length_remaining <= 0)
				{
					if (!this->read_packet(this->packet_buffer))
						return false;
					this->reset_reader();
				}
			}
		}
	}
	return true;
}

bool wild::client::read_packet(std::vector<uint8_t> data)
{
	auto needle = this->packet_buffer.begin();
	auto end = this->packet_buffer.end();
	std::optional<int32_t> id = read_fn::read_varint(needle, end);
	if (!id.has_value())
		return false;

	const packet_form *form;
	bool found_form = false;
	for (int i = 0; i < FORMS_SIZE; i++)
	{
		form = &forms[i];
		if (form->id == id && this->state == form->state)
		{
			found_form = true;
			break;
		}
	}
	if (!found_form)
	{
		//KICK_CLIENT;
		return true;
	}

	packet packet;
	packet.name = form->name;
	packet.form = form;
#define KICK_FOR_MALFORMED this->server.client_malformed_packet(this);\
			return false;
	for (int i = 0; i < form->num_fields; i++)
	{
		packet_form::field field = form->fields[i];
		if (needle == end)
		{
			KICK_FOR_MALFORMED
		}
		switch (field.type)
		{
			case data_type::BOOL:
			{
				auto res = read_fn::read_bool(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::BYTE:
			{
				auto res = read_fn::read_i8(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::UNSIGNED_BYTE:
			{
				auto res = read_fn::read_u8(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::SHORT:
			{
				auto res = read_fn::read_i16(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::UNSIGNED_SHORT:
			{
				auto res = read_fn::read_u16(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::INT:
			{
				auto res = read_fn::read_i32(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::LONG:
			{
				auto res = read_fn::read_i64(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::FLOAT:
			{
				auto res = read_fn::read_float(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::DOUBLE:
			{
				auto res = read_fn::read_double(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::STRING:
			{
				auto res = read_fn::read_string(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			case data_type::VARINT:
			{
				auto res = read_fn::read_varint(needle, end);
				if (!res.has_value())
				{
					KICK_FOR_MALFORMED
				}

				packet.data.emplace(field.name, res.value());
			}
			break;
			default:
				break;
		}
	}

	this->server.handle_client_packet(this, packet);
	return true;
}

void wild::client::reset_reader()
{
	this->read_state = READ_LENGTH_VARINT;
	this->packet_length_reader.reset();
	this->packet_length_remaining = 0;
	this->packet_buffer.clear();
}

void wild::client::start_read()
{
	bool dc = false;
	while (!dc)
	{
		try
		{
			size_t size = this->socket.read_some(asio::buffer(this->read_buf, READ_BUFFER_SIZE));
			std::vector<uint8_t> data(this->read_buf, this->read_buf + size);
			dc = !this->handle_data(data);
		} catch (asio::system_error ec)
		{
			if (ec.code() == asio::error::eof)
			{
				dc = true;
			}
		}
	}
	this->server.client_disconnected(this);
}

void wild::client::handle_write(asio::error_code ec)
{
	if (ec)
	{
		PLOGF << "Error writing: " << ec.message();
		this->server.client_disconnected(this);
		return;
	}
	this->write_queue_mutex.lock();
	this->write_in_progress = false;
	if (!this->write_queue.empty())
	{
		this->write_in_progress = true;
		asio::async_write(this->socket, asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()), std::bind(&wild::client::handle_write, this, std::placeholders::_1));
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}

void wild::client::send_data(const std::vector<uint8_t> &data)
{
	this->write_queue.push_back(data);
	if (this->write_queue.size() <= 1)
	{
		this->write_in_progress = true;
		asio::async_write(this->socket, asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()),
			std::bind(&wild::client::handle_write, this, std::placeholders::_1));
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}
void wild::client::send_packet(const wild::clientbound_packet &packet)
{
	this->write_queue_mutex.lock();
	//tod
	std::vector<uint8_t> data = packet.package();
	this->write_queue.push_back(data);
	if (this->write_queue.size() <= 1)
	{
		this->write_in_progress = true;
		asio::async_write(this->socket, asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()),
			std::bind(&wild::client::handle_write, this, std::placeholders::_1));
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}

void wild::client::send_packet(const wild::clientbound_packet &packet, std::function<void(wild::client *)> callback)
{
	this->write_queue_mutex.lock();
	std::vector<uint8_t> data = packet.package();
	this->write_queue.push_back(data);
	if (this->write_queue.size() <= 1)
	{
		this->write_in_progress = true;
		asio::async_write(this->socket, asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()),
			[this, callback](asio::error_code ec, int i)
			{
				callback(this);
				this->handle_write(ec);
			});
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}

void wild::client::receive_packet(const wild::packet &packet)
{
	static std::map<std::tuple<wild::client_state, uint8_t>, packet_callback> callback_map = {
		 {{wild::client_state::HANDSHAKING, 0x00}, &wild::client::Handle_HANDSHAKING_Handshake},
		 {{wild::client_state::STATUS,      0x00}, &wild::client::Handle_STATUS_Request},
		 {{wild::client_state::STATUS,      0x01}, &wild::client::Handle_STATUS_Ping},
		 {{wild::client_state::LOGIN,       0x00}, &wild::client::Handle_LOGIN_LoginStart},
		 {{wild::client_state::PLAY,        0x00}, &wild::client::Handle_PLAY_KeepAlive},
		 {{wild::client_state::PLAY,        0x01}, &wild::client::Handle_PLAY_ChatMessage},
		 {{wild::client_state::PLAY,        0x02}, &wild::client::Handle_PLAY_UseEntity},
		 {{wild::client_state::PLAY,        0x03}, &wild::client::Handle_PLAY_Player},
		 {{wild::client_state::PLAY,        0x04}, &wild::client::Handle_PLAY_PlayerPosition},
		 {{wild::client_state::PLAY,        0x05}, &wild::client::Handle_PLAY_PlayerLook},
		 {{wild::client_state::PLAY,        0x06}, &wild::client::Handle_PLAY_PlayerPositionAndLook},
		 {{wild::client_state::PLAY,        0x07}, &wild::client::Handle_PLAY_PlayerDigging},
		 {{wild::client_state::PLAY,        0x09}, &wild::client::Handle_PLAY_HeldItemChange},
		 {{wild::client_state::PLAY,        0x0A}, &wild::client::Handle_PLAY_Animation},
		 {{wild::client_state::PLAY,        0x0B}, &wild::client::Handle_PLAY_EntityAction},
		 {{wild::client_state::PLAY,        0x0C}, &wild::client::Handle_PLAY_SteerVehicle},
		 {{wild::client_state::PLAY,        0x0D}, &wild::client::Handle_PLAY_CloseWindow},
		 {{wild::client_state::PLAY,        0x0F}, &wild::client::Handle_PLAY_ConfirmTransaction},
		 {{wild::client_state::PLAY,        0x11}, &wild::client::Handle_PLAY_EnchantItem},
		 {{wild::client_state::PLAY,        0x12}, &wild::client::Handle_PLAY_UpdateSign},
		 {{wild::client_state::PLAY,        0x13}, &wild::client::Handle_PLAY_PlayerAbilities},
		 {{wild::client_state::PLAY,        0x14}, &wild::client::Handle_PLAY_TabComplete},
		 {{wild::client_state::PLAY,        0x15}, &wild::client::Handle_PLAY_ClientSettings},
		 {{wild::client_state::PLAY,        0x16}, &wild::client::Handle_PLAY_ClientStatus},
	};

	//is this inefficient ?
	(this->*(callback_map[std::make_tuple(this->state, packet.form->id)]))(packet);
}

void wild::client::do_keepalive()
{
	this->keepalive_id = Random::random_i32();
	wild::clientbound_packet keepalive(0x00);
	keepalive.write_i32(this->keepalive_id);
	this->send_packet(keepalive);
}

void wild::client::kick_if_keepalive_expired()
{
	using namespace std::chrono_literals;
	if ((std::chrono::high_resolution_clock::now() - this->last_time_since_keepalive) > 20s)
	{
		//todo: get strings language file
		this->kick("No KeepAlive ID!");
	}
}

void wild::client::start_keepalive()
{
	this->keepalive_runnable_id = this->server.game.create_c_runnable([this](uint32_t runnable_id)
		{
			this->do_keepalive();
			return true;
		}, std::make_tuple(wild::runnable::run_type::TIMER, 0, 10));
}

void wild::client::kick(std::string reason)
{
	wild::clientbound_packet disconnect(0x40);

	nlohmann::json kick_reason = { {"text", reason} };

	disconnect.write_string(kick_reason.dump());

	this->send_packet(disconnect, [](wild::client *client)
		{
			client->server.client_disconnected(client);
		});
}

wild::client::~client()
{
	if (this->keepalive_runnable_id.has_value())
	{
		this->server.game.stop_runnable(this->keepalive_runnable_id.value());
	}
	this->socket.shutdown(asio::ip::tcp::socket::shutdown_both);
	this->socket.cancel();
	this->socket.close();
}

void wild::client::Handle_HANDSHAKING_Handshake(const wild::packet &packet)
{
	//(5 as of 1.7.10)
	int32_t protocol_version = std::get<int32_t>(packet.data.at("protocol_version"));
	//localhost
	std::string server_address = std::get<std::string>(packet.data.at("server_address"));
	//25565
	uint16_t server_port = std::get<uint16_t>(packet.data.at("server_port"));
	//1 for status, 2 for login
	int32_t next_state = std::get<int32_t>(packet.data.at("next_state"));

	if (next_state == 1)
	{
		this->state = client_state::STATUS;
	} else if (next_state == 2)
	{
		this->state = client_state::LOGIN;
	}
}

void wild::client::Handle_STATUS_Request(const wild::packet &packet)
{
	wild::clientbound_packet response_packet(0x00);
	nlohmann::json response =
	{
	{"version", {
		{"name", "wild v1"},
		{"protocol", 5}
	}},
	{"players", {
		{"max", 100},
		{"online", 5},
		{"sample", {}}
	}},
	{"description", {
		{"text", "Hello World"}
	}}
	};

	response_packet.write_string(response.dump());
	this->send_packet(response_packet);
}
void wild::client::Handle_STATUS_Ping(const wild::packet &packet)
{
	int64_t time = std::get<int64_t>(packet.data.at("time"));

	wild::clientbound_packet pong(0x01);
	pong.write_i64(time);
	this->send_packet(pong);
}
void wild::client::Handle_LOGIN_LoginStart(const wild::packet &packet)
{
	std::string name = std::get<std::string>(packet.data.at("name"));

	wild::clientbound_packet login_success(0x02);
	login_success.write_string("9e59c4b1-7a70-4b01-abba-57d87d1e0c2b");
	login_success.write_string("eric_Yale");

	this->send_packet(login_success);
	this->state = client_state::PLAY;

	this->start_keepalive();
	//todo: spawn player in
	this->player = new wild::player(*this);
	wild::game_event join_game_event = wild::game_event::player_join(this->player);
	this->server.game.queue_event(join_game_event);
}
void wild::client::Handle_PLAY_KeepAlive(const wild::packet &packet)
{
	int32_t client_keepalive_id = std::get<int32_t>(packet.data.at("keep_alive_id"));
	if (this->keepalive_id != client_keepalive_id)
	{
		this->kick("Incorrect KeepAlive ID!");
	}
	this->last_time_since_keepalive = std::chrono::high_resolution_clock::now();
}
void wild::client::Handle_PLAY_ChatMessage(const wild::packet &packet)
{
	std::string message = std::get<std::string>(packet.data.at("message"));
}
void wild::client::Handle_PLAY_UseEntity(const wild::packet &packet)
{
	int32_t target = std::get<int32_t>(packet.data.at("target"));
	//0 = Right-click, 1 = Left-click
	int8_t mouse = std::get<int8_t>(packet.data.at("mouse"));
}
void wild::client::Handle_PLAY_Player(const wild::packet &packet)
{
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet.data.at("on_ground"));
}
void wild::client::Handle_PLAY_PlayerPosition(const wild::packet &packet)
{
	//Absolute position
	double x = std::get<double>(packet.data.at("x"));
	//Absolute feet position, normally HeadY - 1.62. Used to modify the players bounding box when going up stairs, crouching, etc…
	double feety = std::get<double>(packet.data.at("feety"));
	//Absolute head position
	double heady = std::get<double>(packet.data.at("heady"));
	//Absolute position
	double z = std::get<double>(packet.data.at("z"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet.data.at("on_ground"));
}
void wild::client::Handle_PLAY_PlayerLook(const wild::packet &packet)
{
	//Absolute rotation on the X Axis, in degrees
	float yaw = std::get<float>(packet.data.at("yaw"));
	//Absolute rotation on the Y Axis, in degrees
	float pitch = std::get<float>(packet.data.at("pitch"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet.data.at("on_ground"));
}
void wild::client::Handle_PLAY_PlayerPositionAndLook(const wild::packet &packet)
{
	//Absolute position
	double x = std::get<double>(packet.data.at("x"));
	//Absolute feet position. Is normally HeadY - 1.62. Used to modify the players bounding box when going up stairs, crouching, etc…
	double feety = std::get<double>(packet.data.at("feety"));
	//Absolute head position
	double heady = std::get<double>(packet.data.at("heady"));
	//Absolute position
	double z = std::get<double>(packet.data.at("z"));
	//Absolute rotation on the X Axis, in degrees
	float yaw = std::get<float>(packet.data.at("yaw"));
	//Absolute rotation on the Y Axis, in degrees
	float pitch = std::get<float>(packet.data.at("pitch"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet.data.at("on_ground"));
}
void wild::client::Handle_PLAY_PlayerDigging(const wild::packet &packet)
{
	//The action the player is taking against the block
	int8_t status = std::get<int8_t>(packet.data.at("status"));
	//Block position
	int32_t x = std::get<int32_t>(packet.data.at("x"));
	//Block position
	uint8_t y = std::get<uint8_t>(packet.data.at("y"));
	//Block position
	int32_t z = std::get<int32_t>(packet.data.at("z"));
	//The face being hit
	int8_t face = std::get<int8_t>(packet.data.at("face"));
}
void wild::client::Handle_PLAY_HeldItemChange(const wild::packet &packet)
{
	//The slot which the player has selected (0-8)
	int16_t slot = std::get<int16_t>(packet.data.at("slot"));
}
void wild::client::Handle_PLAY_Animation(const wild::packet &packet)
{
	//Player ID
	int32_t entity_id = std::get<int32_t>(packet.data.at("entity_id"));
	//Animation ID
	int8_t animation = std::get<int8_t>(packet.data.at("animation"));
}
void wild::client::Handle_PLAY_EntityAction(const wild::packet &packet)
{
	//Player ID
	int32_t entity_id = std::get<int32_t>(packet.data.at("entity_id"));
	//The ID of the action
	int8_t action_id = std::get<int8_t>(packet.data.at("action_id"));
	//Horse jump boost. Ranged from 0 -> 100.
	int32_t jump_boost = std::get<int32_t>(packet.data.at("jump_boost"));
}
void wild::client::Handle_PLAY_SteerVehicle(const wild::packet &packet)
{
	//Positive to the left of the player
	float sideways = std::get<float>(packet.data.at("sideways"));
	//Positive forward
	float forward = std::get<float>(packet.data.at("forward"));
	bool jump = std::get<bool>(packet.data.at("jump"));
	//True when leaving the vehicle
	bool unmount = std::get<bool>(packet.data.at("unmount"));
}
void wild::client::Handle_PLAY_CloseWindow(const wild::packet &packet)
{
	//This is the id of the window that was closed. 0 for inventory.
	int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
}
void wild::client::Handle_PLAY_ConfirmTransaction(const wild::packet &packet)
{
	//The id of the window that the action occurred in.
	int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
	//Every action that is to be accepted has a unique number. This field corresponds to that number.
	int16_t action_number = std::get<int16_t>(packet.data.at("action_number"));
	//Whether the action was accepted.
	bool accepted = std::get<bool>(packet.data.at("accepted"));
}
void wild::client::Handle_PLAY_EnchantItem(const wild::packet &packet)
{
	//The ID sent by Open Window
	int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
	//The position of the enchantment on the enchantment table window, starting with 0 as the topmost one.
	int8_t enchantment = std::get<int8_t>(packet.data.at("enchantment"));
}
void wild::client::Handle_PLAY_UpdateSign(const wild::packet &packet)
{
	//Block X Coordinate
	int32_t x = std::get<int32_t>(packet.data.at("x"));
	//Block Y Coordinate
	int16_t y = std::get<int16_t>(packet.data.at("y"));
	//Block Z Coordinate
	int32_t z = std::get<int32_t>(packet.data.at("z"));
	//First line of text in the sign
	std::string line_1 = std::get<std::string>(packet.data.at("line_1"));
	//Second line of text in the sign
	std::string line_2 = std::get<std::string>(packet.data.at("line_2"));
	//Third line of text in the sign
	std::string line_3 = std::get<std::string>(packet.data.at("line_3"));
	//Fourth line of text in the sign
	std::string line_4 = std::get<std::string>(packet.data.at("line_4"));
}
void wild::client::Handle_PLAY_PlayerAbilities(const wild::packet &packet)
{
	int8_t flags = std::get<int8_t>(packet.data.at("flags"));
	//previous integer value divided by 250
	float flying_speed = std::get<float>(packet.data.at("flying_speed"));
	//previous integer value divided by 250
	float walking_speed = std::get<float>(packet.data.at("walking_speed"));
}
void wild::client::Handle_PLAY_TabComplete(const wild::packet &packet)
{
	std::string text = std::get<std::string>(packet.data.at("text"));
}
void wild::client::Handle_PLAY_ClientSettings(const wild::packet &packet)
{
	//en_GB
	std::string locale = std::get<std::string>(packet.data.at("locale"));
	//Client-side render distance(chunks)
	int8_t view_distance = std::get<int8_t>(packet.data.at("view_distance"));
	//Chat settings
	int8_t chat_flags = std::get<int8_t>(packet.data.at("chat_flags"));
	//"Colours" multiplayer setting
	bool chat_colours = std::get<bool>(packet.data.at("chat_colours"));
	//Client-side difficulty from options.txt
	int8_t difficulty = std::get<int8_t>(packet.data.at("difficulty"));
	//Show Cape multiplayer setting
	bool show_cape = std::get<bool>(packet.data.at("show_cape"));
}
void wild::client::Handle_PLAY_ClientStatus(const wild::packet &packet)
{
	int8_t action_id = std::get<int8_t>(packet.data.at("action_id"));
}
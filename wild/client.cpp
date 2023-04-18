#include <asio.hpp>
#include <cassert>
#include <tuple>
#include <nlohmann/json.hpp>
#include <cstddef>
#include <zlib.h>
#include <plog/Log.h>
#include "random.h"
#include "server.h"
#include "client.h"
#include "packet.h"
#include "clientbound_packet.h"

typedef void(w_client:: *packet_callback)(const w_packet *packet);

std::map<std::tuple<w_client::client_state, uint8_t>, packet_callback> callback_map = {
	{{w_client::client_state::HANDSHAKING, 0x00}, &w_client::Handle_HANDSHAKING_Handshake},
	{{w_client::client_state::PLAY,        0x00}, &w_client::Handle_PLAY_KeepAlive},
	{{w_client::client_state::PLAY,        0x01}, &w_client::Handle_PLAY_ChatMessage},
	{{w_client::client_state::PLAY,        0x02}, &w_client::Handle_PLAY_UseEntity},
	{{w_client::client_state::PLAY,        0x03}, &w_client::Handle_PLAY_Player},
	{{w_client::client_state::PLAY,        0x04}, &w_client::Handle_PLAY_PlayerPosition},
	{{w_client::client_state::PLAY,        0x05}, &w_client::Handle_PLAY_PlayerLook},
	{{w_client::client_state::PLAY,        0x06}, &w_client::Handle_PLAY_PlayerPositionAndLook},
	{{w_client::client_state::PLAY,        0x07}, &w_client::Handle_PLAY_PlayerDigging},
	{{w_client::client_state::PLAY,        0x09}, &w_client::Handle_PLAY_HeldItemChange},
	{{w_client::client_state::PLAY,        0x0A}, &w_client::Handle_PLAY_Animation},
	{{w_client::client_state::PLAY,        0x0B}, &w_client::Handle_PLAY_EntityAction},
	{{w_client::client_state::PLAY,        0x0C}, &w_client::Handle_PLAY_SteerVehicle},
	{{w_client::client_state::PLAY,        0x0D}, &w_client::Handle_PLAY_CloseWindow},
	{{w_client::client_state::PLAY,        0x0F}, &w_client::Handle_PLAY_ConfirmTransaction},
	{{w_client::client_state::PLAY,        0x11}, &w_client::Handle_PLAY_EnchantItem},
	{{w_client::client_state::PLAY,        0x12}, &w_client::Handle_PLAY_UpdateSign},
	{{w_client::client_state::PLAY,        0x13}, &w_client::Handle_PLAY_PlayerAbilities},
	{{w_client::client_state::PLAY,        0x14}, &w_client::Handle_PLAY_TabComplete},
	{{w_client::client_state::PLAY,        0x15}, &w_client::Handle_PLAY_ClientSettings},
	{{w_client::client_state::PLAY,        0x16}, &w_client::Handle_PLAY_ClientStatus},
	{{w_client::client_state::STATUS,      0x00}, &w_client::Handle_STATUS_Request},
	{{w_client::client_state::STATUS,      0x01}, &w_client::Handle_STATUS_Ping},
	{{w_client::client_state::LOGIN,       0x00}, &w_client::Handle_LOGIN_LoginStart},
};

w_client::w_client(tcp::socket *socket, w_server *server) : server(server), socket(socket)
{
	this->read_stream = new w_packet_read_stream(this);
}

bool w_client::handle_data(const std::vector<uint8_t> &data)
{
	return this->read_stream->handle_data(data);
}

void w_client::reset_reader()
{
	delete this->read_stream;
	this->read_stream = new w_packet_read_stream(this);
}

void w_client::start_read()
{
	bool dc = false;
	while (!dc)
	{
		try
		{
			size_t size = this->socket->read_some(asio::buffer(this->read_buf, READ_BUFFER_SIZE));
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
	this->server->client_disconnected(this);
}

void w_client::handle_write(asio::error_code ec)
{
	if (ec)
	{
		PLOGF << "Error writing.";
	}
	this->write_queue_mutex.lock();
	this->write_in_progress = false;
	if (!this->write_queue.empty())
	{
		this->write_in_progress = true;
		asio::async_write(*(this->socket), asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()), std::bind(&w_client::handle_write, this, std::placeholders::_1));
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}

void w_client::send_packet(const w_clientbound_packet *packet)
{
	this->write_queue_mutex.lock();
	//tod
	std::vector<uint8_t> id_varint = write_fn::write_varint(packet->id);
	std::vector<uint8_t> length_varint = write_fn::write_varint(id_varint.size() + packet->data.size());

	std::vector<uint8_t> all;
	all.insert(all.begin(), length_varint.begin(), length_varint.end());
	all.insert(all.end(), id_varint.begin(), id_varint.end());
	all.insert(all.end(), packet->data.begin(), packet->data.end());

	this->write_queue.push_back(all);
	if (this->write_queue.size() <= 1)
	{
		this->write_in_progress = true;
		asio::async_write(*(this->socket), asio::buffer(this->write_queue.front().data(), this->write_queue.front().size()), std::bind(&w_client::handle_write, this, std::placeholders::_1));
		this->write_queue.pop_front();
	}
	this->write_queue_mutex.unlock();
}

void w_client::send_packet(const w_clientbound_packet *packet, std::function<void(w_client *)> callback)
{
	std::vector<uint8_t> id_varint = write_fn::write_varint(packet->id);
	std::vector<uint8_t> length_varint = write_fn::write_varint(id_varint.size() + packet->data.size());

	std::vector<uint8_t> all;
	all.insert(all.begin(), length_varint.begin(), length_varint.end());
	all.insert(all.end(), id_varint.begin(), id_varint.end());
	all.insert(all.end(), packet->data.begin(), packet->data.end());

	asio::async_write(*(this->socket), asio::buffer(all), std::bind(callback, this));
}

void w_client::receive_packet(const w_packet *packet)
{
	(this->*(callback_map[std::make_tuple(this->state, packet->form->id)]))(packet);
}

void w_client::do_keepalive(uint32_t runnable_id)
{
	this->keepalive_runnable_id = runnable_id;
	w_clientbound_packet keepalive(0x00);
	this->keepalive_id = Random::random_i32();
	keepalive.write_i32(this->keepalive_id);
	this->send_packet(&keepalive);
}
void w_client::kick()
{
	w_clientbound_packet disconnect(0x40);

	nlohmann::json kick_reason = { {"text", "You have been kicked"} };

	disconnect.write_string(kick_reason.dump());

	this->send_packet(&disconnect, [](w_client *client)
		{
			client->server->client_disconnected(client);
		});
}

w_client::~w_client()
{
	delete this->read_stream;
	this->socket->cancel();
	this->socket->close();
	this->server->game.lua_vm.stop_runnable(this->keepalive_runnable_id);
	delete this->socket;
}

void w_client::Handle_HANDSHAKING_Handshake(const w_packet *packet)
{
	//(5 as of 1.7.10)
	int32_t protocol_version = std::get<int32_t>(packet->data.at("protocol_version"));
	//localhost
	std::string server_address = std::get<std::string>(packet->data.at("server_address"));
	//25565
	uint16_t server_port = std::get<uint16_t>(packet->data.at("server_port"));
	//1 for status, 2 for login
	int32_t next_state = std::get<int32_t>(packet->data.at("next_state"));

	if (next_state == 1)
	{
		this->state = client_state::STATUS;
	} else if (next_state == 2)
	{
		this->state = client_state::LOGIN;
	}
}
void w_client::Handle_STATUS_Request(const w_packet *packet)
{
	w_clientbound_packet response_packet(0x00);
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
	this->send_packet(&response_packet);
}
void w_client::Handle_STATUS_Ping(const w_packet *packet)
{
	int64_t time = std::get<int64_t>(packet->data.at("time"));

	w_clientbound_packet pong(0x01);
	pong.write_i64(time);
	this->send_packet(&pong);
}
void w_client::Handle_LOGIN_LoginStart(const w_packet *packet)
{
	std::string name = std::get<std::string>(packet->data.at("name"));

	w_clientbound_packet login_success(0x02);
	login_success.write_string("9e59c4b1-7a70-4b01-abba-57d87d1e0c2b");
	login_success.write_string("eric_Yale");

	this->send_packet(&login_success);

	this->spawn_player_in();
	this->send_chunk_updates();
}
void w_client::Handle_PLAY_KeepAlive(const w_packet *packet)
{
	int32_t keep_alive_id = std::get<int32_t>(packet->data.at("keep_alive_id"));
	if (keep_alive_id != this->keepalive_id)
	{
		this->kick();
	}
}
void w_client::Handle_PLAY_ChatMessage(const w_packet *packet)
{
	std::string message = std::get<std::string>(packet->data.at("message"));
}
void w_client::Handle_PLAY_UseEntity(const w_packet *packet)
{
	int32_t target = std::get<int32_t>(packet->data.at("target"));
	//0 = Right-click, 1 = Left-click
	int8_t mouse = std::get<int8_t>(packet->data.at("mouse"));
}
void w_client::Handle_PLAY_Player(const w_packet *packet)
{
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet->data.at("on_ground"));
}
void w_client::Handle_PLAY_PlayerPosition(const w_packet *packet)
{
	//Absolute position
	double x = std::get<double>(packet->data.at("x"));
	//Absolute feet position, normally HeadY - 1.62. Used to modify the players bounding box when going up stairs, crouching, etc…
	double feety = std::get<double>(packet->data.at("feety"));
	//Absolute head position
	double heady = std::get<double>(packet->data.at("heady"));
	//Absolute position
	double z = std::get<double>(packet->data.at("z"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet->data.at("on_ground"));
}
void w_client::Handle_PLAY_PlayerLook(const w_packet *packet)
{
	//Absolute rotation on the X Axis, in degrees
	float yaw = std::get<float>(packet->data.at("yaw"));
	//Absolute rotation on the Y Axis, in degrees
	float pitch = std::get<float>(packet->data.at("pitch"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet->data.at("on_ground"));
}
void w_client::Handle_PLAY_PlayerPositionAndLook(const w_packet *packet)
{
	//Absolute position
	double x = std::get<double>(packet->data.at("x"));
	//Absolute feet position. Is normally HeadY - 1.62. Used to modify the players bounding box when going up stairs, crouching, etc…
	double feety = std::get<double>(packet->data.at("feety"));
	//Absolute head position
	double heady = std::get<double>(packet->data.at("heady"));
	//Absolute position
	double z = std::get<double>(packet->data.at("z"));
	//Absolute rotation on the X Axis, in degrees
	float yaw = std::get<float>(packet->data.at("yaw"));
	//Absolute rotation on the Y Axis, in degrees
	float pitch = std::get<float>(packet->data.at("pitch"));
	//True if the client is on the ground, False otherwise
	bool on_ground = std::get<bool>(packet->data.at("on_ground"));
}
void w_client::Handle_PLAY_PlayerDigging(const w_packet *packet)
{
	//The action the player is taking against the block
	int8_t status = std::get<int8_t>(packet->data.at("status"));
	//Block position
	int32_t x = std::get<int32_t>(packet->data.at("x"));
	//Block position
	uint8_t y = std::get<uint8_t>(packet->data.at("y"));
	//Block position
	int32_t z = std::get<int32_t>(packet->data.at("z"));
	//The face being hit
	int8_t face = std::get<int8_t>(packet->data.at("face"));
}
void w_client::Handle_PLAY_HeldItemChange(const w_packet *packet)
{
	//The slot which the player has selected (0-8)
	int16_t slot = std::get<int16_t>(packet->data.at("slot"));
}
void w_client::Handle_PLAY_Animation(const w_packet *packet)
{
	//Player ID
	int32_t entity_id = std::get<int32_t>(packet->data.at("entity_id"));
	//Animation ID
	int8_t animation = std::get<int8_t>(packet->data.at("animation"));
}
void w_client::Handle_PLAY_EntityAction(const w_packet *packet)
{
	//Player ID
	int32_t entity_id = std::get<int32_t>(packet->data.at("entity_id"));
	//The ID of the action
	int8_t action_id = std::get<int8_t>(packet->data.at("action_id"));
	//Horse jump boost. Ranged from 0 -> 100.
	int32_t jump_boost = std::get<int32_t>(packet->data.at("jump_boost"));
}
void w_client::Handle_PLAY_SteerVehicle(const w_packet *packet)
{
	//Positive to the left of the player
	float sideways = std::get<float>(packet->data.at("sideways"));
	//Positive forward
	float forward = std::get<float>(packet->data.at("forward"));
	bool jump = std::get<bool>(packet->data.at("jump"));
	//True when leaving the vehicle
	bool unmount = std::get<bool>(packet->data.at("unmount"));
}
void w_client::Handle_PLAY_CloseWindow(const w_packet *packet)
{
	//This is the id of the window that was closed. 0 for inventory.
	int8_t window_id = std::get<int8_t>(packet->data.at("window_id"));
}
void w_client::Handle_PLAY_ConfirmTransaction(const w_packet *packet)
{
	//The id of the window that the action occurred in.
	int8_t window_id = std::get<int8_t>(packet->data.at("window_id"));
	//Every action that is to be accepted has a unique number. This field corresponds to that number.
	int16_t action_number = std::get<int16_t>(packet->data.at("action_number"));
	//Whether the action was accepted.
	bool accepted = std::get<bool>(packet->data.at("accepted"));
}
void w_client::Handle_PLAY_EnchantItem(const w_packet *packet)
{
	//The ID sent by Open Window
	int8_t window_id = std::get<int8_t>(packet->data.at("window_id"));
	//The position of the enchantment on the enchantment table window, starting with 0 as the topmost one.
	int8_t enchantment = std::get<int8_t>(packet->data.at("enchantment"));
}
void w_client::Handle_PLAY_UpdateSign(const w_packet *packet)
{
	//Block X Coordinate
	int32_t x = std::get<int32_t>(packet->data.at("x"));
	//Block Y Coordinate
	int16_t y = std::get<int16_t>(packet->data.at("y"));
	//Block Z Coordinate
	int32_t z = std::get<int32_t>(packet->data.at("z"));
	//First line of text in the sign
	std::string line_1 = std::get<std::string>(packet->data.at("line_1"));
	//Second line of text in the sign
	std::string line_2 = std::get<std::string>(packet->data.at("line_2"));
	//Third line of text in the sign
	std::string line_3 = std::get<std::string>(packet->data.at("line_3"));
	//Fourth line of text in the sign
	std::string line_4 = std::get<std::string>(packet->data.at("line_4"));
}
void w_client::Handle_PLAY_PlayerAbilities(const w_packet *packet)
{
	int8_t flags = std::get<int8_t>(packet->data.at("flags"));
	//previous integer value divided by 250
	float flying_speed = std::get<float>(packet->data.at("flying_speed"));
	//previous integer value divided by 250
	float walking_speed = std::get<float>(packet->data.at("walking_speed"));
}
void w_client::Handle_PLAY_TabComplete(const w_packet *packet)
{
	std::string text = std::get<std::string>(packet->data.at("text"));
}
void w_client::Handle_PLAY_ClientSettings(const w_packet *packet)
{
	//en_GB
	std::string locale = std::get<std::string>(packet->data.at("locale"));
	//Client-side render distance(chunks)
	int8_t view_distance = std::get<int8_t>(packet->data.at("view_distance"));
	//Chat settings
	int8_t chat_flags = std::get<int8_t>(packet->data.at("chat_flags"));
	//"Colours" multiplayer setting
	bool chat_colours = std::get<bool>(packet->data.at("chat_colours"));
	//Client-side difficulty from options.txt
	int8_t difficulty = std::get<int8_t>(packet->data.at("difficulty"));
	//Show Cape multiplayer setting
	bool show_cape = std::get<bool>(packet->data.at("show_cape"));
}
void w_client::Handle_PLAY_ClientStatus(const w_packet *packet)
{
	int8_t action_id = std::get<int8_t>(packet->data.at("action_id"));
}

void client_keepalive_cb(void *client_ptr, uint32_t runnable_id)
{
	reinterpret_cast<w_client *>(client_ptr)->do_keepalive(runnable_id);
}

void w_client::spawn_player_in()
{
	w_clientbound_packet join_game(0x01);

	join_game.write_i32(1);
	join_game.write_i8(0);
	join_game.write_i8(0);
	join_game.write_u8(0);
	join_game.write_u8(0);
	join_game.write_string("default");

	w_clientbound_packet player_abilities(0x39);

	player_abilities.write_i8(1);
	player_abilities.write_float((float)1 / (float)250);
	player_abilities.write_float((float)1 / (float)250);

	w_clientbound_packet spawn_position(0x05);
	spawn_position.write_i32(0);
	spawn_position.write_i32(0);
	spawn_position.write_i32(0);

	w_clientbound_packet player_position_and_look(0x08);
	player_position_and_look.write_double(8);
	player_position_and_look.write_double(194);
	player_position_and_look.write_double(8);

	player_position_and_look.write_float(0);
	player_position_and_look.write_float(0);

	player_position_and_look.write_bool(false);

	this->send_packet(&join_game);
	this->send_packet(&player_abilities);
	this->send_packet(&spawn_position);
	this->send_packet(&player_position_and_look);

	this->state = client_state::PLAY;

	w_runnable *runnable = new w_runnable(&this->server->game.lua_vm, client_keepalive_cb, this);
	runnable->run_timer(0, 20 * 15);
}

void w_client::send_chunk_updates()
{
	w_clientbound_packet chunk_data(0x21);
	chunk_data.write_i32(0);
	chunk_data.write_i32(0);
	chunk_data.write_bool(true);
	chunk_data.write_i16(0b0000100000000000);
	chunk_data.write_i16(0);

	std::vector<uint8_t> data;
	int num_chunks = 1;

	uint8_t metadata = 0;
	uint8_t block_light = 0;
	uint8_t skylight = 0;
	uint8_t add = 0;

	for (int i = 0; i < 16 * 16 * 16 * num_chunks; i++)
	{
		data.push_back(4);
	}
	for (int i = 0; i < 16 * 16 * 16 * num_chunks; i++)
	{
		data.push_back((metadata << 4) | block_light);
	}
	for (int i = 0; i < 16 * 16 * 16 * num_chunks; i++)
	{
		data.push_back((skylight << 4) | add);
	}
	for (int i = 0; i < 256; i++)
	{
		data.push_back(7);
	}

	uLong compressed_size = compressBound(data.size());
	uint8_t *compressed = new uint8_t[compressed_size];
	compress(compressed, &compressed_size, data.data(), data.size());
	auto vec = std::vector<uint8_t>(compressed, compressed + compressed_size);
	delete[] compressed;

	chunk_data.write_i32(vec.size());
	for (auto it = vec.begin(); it != vec.end(); it++)
	{
		chunk_data.write_i8(*it);
	}

	w_clientbound_packet time_update(0x03);
	time_update.write_i64(20000);
	time_update.write_i64(18000);

	this->send_packet(&chunk_data);
	this->send_packet(&time_update);
}
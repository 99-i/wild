#pragma once
#include <uv.h>
#include <vector>
#include <sol/sol.hpp>
#include <map>

#include "player.h"

struct w_server;
struct w_packet_read_stream;
struct lua_clientbound_packet;
struct w_client
{
	enum class client_state
	{
		HANDSHAKING,
		STATUS,
		LOGIN,
		PLAY
	} state = client_state::HANDSHAKING;

	w_server *server;

	w_player player;

	w_packet_read_stream *read_stream;
	void handle_data(const std::vector<uint8_t> &data);

	void reset_reader();

	uv_tcp_t tcp_handle;

	void send_packet(lua_clientbound_packet *packet);

	bool disconnect = false;

	void disconnect_client();
	w_client(w_server *server);
	~w_client();

	std::map<std::string, sol::object> lua_obj_data;
	std::string get_state_str();
	void set_state_str(std::string str);
};

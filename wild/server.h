#pragma once
#include <array>
#include <asio.hpp>
#include <thread>
#include <mutex>

#include "game.h"

struct w_client;
struct w_packet;

using asio::ip::tcp;
struct w_server
{
	static constexpr uint32_t max_players = 100;
	bool running = false;
	int port;

	asio::io_context context;
	tcp::acceptor acceptor;

	std::thread accept_thread;
	std::thread game_thread;

	std::mutex game_mutex;
	w_game game;

	void start_accept();

	void handle_accept(tcp::socket *socket, asio::error_code ec);

	void read_from_client(w_client *client, std::vector<uint8_t> data);

	//called when a client sends malformed data
	void client_malformed_packet(w_client *client);

	void client_disconnected(w_client *client);

	void handle_client_packet(w_client *client, w_packet *packet);

	std::vector<w_client *> clients;

	w_server(int port = 25565);
	void run();

	void restart();
};

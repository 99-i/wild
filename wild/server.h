#pragma once
#include <array>
#include <asio.hpp>
#include <thread>
#include <mutex>

#include "game.h"

namespace wild
{
	struct client;
	struct packet;
	struct server
	{
		static constexpr uint32_t max_players = 100;
		bool running = false;
		int port;

		asio::io_context context;
		asio::ip::tcp::acceptor acceptor;

		std::thread accept_thread;
		std::thread game_thread;

		std::mutex game_mutex;
		wild::game game;

		void start_accept();

		void handle_accept(asio::ip::tcp::socket *socket, asio::error_code ec);

		void read_from_client(wild::client *client, std::vector<uint8_t> data);

		//called when a client sends malformed data
		void client_malformed_packet(wild::client *client);

		void client_disconnected(wild::client *client);

		void handle_client_packet(wild::client *client, wild::packet *packet);

		std::mutex clients_mutex;
		std::vector<wild::client *> clients;

		server(int port = 25565);
		void run();

		void restart();
	};
}

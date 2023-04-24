#pragma once
#include <array>
#include <asio.hpp>
#include <thread>
#include <mutex>
#include <optional>

#include "game.h"

namespace wild
{
	class client;
	class packet;

	class server
	{
		static constexpr uint32_t max_players = 100;
		bool running = false;
		int port;

		asio::io_context context;
		asio::ip::tcp::acceptor acceptor;

		std::thread accept_thread;
		std::thread game_thread;

		std::mutex game_mutex;

		std::optional<asio::ip::tcp::socket> socket;
		void start_accept();

		void handle_accept(asio::ip::tcp::socket &socket, asio::error_code ec);

		std::mutex clients_mutex;
		std::vector<wild::client *> clients;

	public:
		server(int port = 25565);
		wild::game game;
		//called by client when a packet is read
		void handle_client_packet(wild::client *client, wild::packet *packet);

		//called when a client sends malformed data
		void client_malformed_packet(wild::client *client);

		void client_disconnected(wild::client *client);
		void run();

		void restart();
	};
}

#pragma once
#include <asio.hpp>
#include <mutex>
#include <net/crypto/rsa_keypair.hpp>
#include <optional>
#include <thread>

#include "../game/game.hpp"

namespace wild
{
	class client;
	class packet;

	class server
	{
		bool running = false;
		uint16_t port;

		asio::io_context context;
		asio::ip::tcp::acceptor acceptor;

	  public:
		std::string server_id;
		rsa_keypair keypair;

	  private:
		void initialize_encryption();
		void create_server_id();

		// thread that handles accepting new clients.
		std::thread accept_thread;
		// thread that runs the game
		std::thread game_thread;

		// mutex for game
		std::mutex game_mutex;

		std::optional<asio::ip::tcp::socket> socket;
		void start_accept();

		void handle_accept(asio::ip::tcp::socket &socket, asio::error_code ec);

		std::mutex clients_mutex;
		std::vector<wild::client *> clients;

	  public:
		static constexpr uint32_t max_players = 100;
		server(uint16_t port = 25565);
		wild::game game;
		// called by client when a packet is read
		void handle_client_packet(wild::client *client, wild::packet &packet);

		// called when a client sends malformed data
		void client_malformed_packet(wild::client *client);

		// called when a client disconnects.
		void client_disconnected(wild::client *client);
		// run the server.
		void run();

		// restart the server.
		void restart();

		void graceful_exit();
	};
} // namespace wild

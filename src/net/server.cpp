#include "net/server.hpp"
#include "net/client.hpp"
#include "net/crypto/rsa_keypair.hpp"
#include "net/packet/packet.hpp"
#include <algorithm>
#include <bcrypt.h>
#include <chrono>
#include <ntstatus.h>
#include <plog/Log.h>
#include <wincrypt.h>
#include <windows.h>

namespace wild
{
	std::string gen_random_string(const int len)
	{
		static const char alphanum[] = "0123456789"
									   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									   "abcdefghijklmnopqrstuvwxyz";
		std::string tmp;
		tmp.reserve(len);

		for (int i = 0; i < len; ++i)
			{
				tmp += alphanum[rand() % (sizeof(alphanum) - 1)];
			}

		return tmp;
	}

	constexpr int SERVER_BACKLOG = 128;

	server::server(uint16_t port)
		: port(port), game(*this),
		  acceptor(context, tcp::endpoint(tcp::v4(), port))
	{
		this->initialize_encryption();
	}

	void server::initialize_encryption()
	{
		this->create_server_id();
		this->keypair.generate();
	}

	void server::create_server_id()
	{
		this->server_id = gen_random_string(13);
		PLOGD << "Server ID:  " << this->server_id;
	}

	void server::run()
	{
		this->game_mutex.lock();
		this->running = true;
		PLOGI << "Running server on port " << port;

		this->accept_thread = std::thread(
			[](server *server) {
				server->start_accept();
				server->context.run();
			},
			this);

		this->game_thread = std::thread(&game::start, &this->game);
		this->game_mutex.unlock();
	}

	void server::restart()
	{
		this->game_mutex.lock();
		auto start = std::chrono::high_resolution_clock::now();
		auto end = std::chrono::high_resolution_clock::now();
		PLOGD << "Server restarted in "
			  << duration_cast<std::chrono::milliseconds>(end - start);
		this->game_mutex.unlock();
	}

	void server::start_accept()
	{
		if (!this->running)
			{
				return;
			}
		socket.emplace(this->context);
		this->acceptor.async_accept(*socket, [&](asio::error_code ec) {
			this->handle_accept(*socket, ec);
		});
	}

	void server::handle_accept(tcp::socket &socket, asio::error_code ec)
	{
		if (!ec)
			{
				client *client = new wild::client(std::move(socket), *this);
				this->clients_mutex.lock();
				this->clients.push_back(client);
				this->clients_mutex.unlock();
				std::thread read_thread(&client::start_read, client);
				read_thread.detach();
			}
		this->start_accept();
	}

	void server::client_malformed_packet(client *client)
	{
		// todo
		client->kick("Malformed Packet!");
	}

	void server::client_disconnected(client *client)
	{
		std::lock_guard<std::mutex> clients_guard{this->clients_mutex};
		if (client->player)
			{
				this->game.queue_event(
					game_event::player_leave(client->player));
			}
		auto index =
			std::find(this->clients.begin(), this->clients.end(), client);
		if (index == this->clients.end())
			return;
		this->clients.erase(index);
		delete client;
	}

	void server::handle_client_packet(client *client, packet &packet)
	{
		client->receive_packet(packet);
	}

	void server::graceful_exit()
	{
		std::lock_guard game_guard{this->game_mutex};
		PLOGD << "Gracefully exiting.";
		this->running = false;
		this->game.stop();
	}
} // namespace wild

#include <plog/Log.h>
#include <chrono>
#include <algorithm>
#include <asio.hpp>
#include "server.h"
#include "client.h"
#include "packet.h"

constexpr int SERVER_BACKLOG = 128;

static void thread_cb(void *server);

w_server::w_server(int port) : port(port), game(this), acceptor(context, tcp::endpoint(tcp::v4(), port))
{
}

void w_server::run()
{
	this->game_mutex.lock();
	this->running = true;
	PLOGI << "Running server on port " << port;

	this->accept_thread = std::thread([](w_server *server)
		{
			server->start_accept();
			server->context.run();
		}, this);

	this->game_thread = std::thread(&w_game::start, &this->game);
	this->game_mutex.unlock();
}

void w_server::restart()
{
	this->game_mutex.lock();
	auto start = std::chrono::high_resolution_clock::now();
	this->game.lua_vm.reset();
	this->game.lua_vm.start();
	auto end = std::chrono::high_resolution_clock::now();
	PLOGD << "Server restarted in " << duration_cast<std::chrono::milliseconds>(end - start);
	this->game_mutex.unlock();
}
void w_server::start_accept()
{
	if (!this->running)
	{
		return;
	}
	tcp::socket *socket = new tcp::socket(this->context);
	this->acceptor.async_accept(*socket, std::bind(&w_server::handle_accept, this, socket, std::placeholders::_1));
}

void w_server::handle_accept(tcp::socket *socket, asio::error_code ec)
{
	if (!ec)
	{
		w_client *client = new w_client(socket, this);
		std::thread read_thread(&w_client::start_read, client);
		read_thread.detach();
		this->clients.push_back(client);
	}
	this->start_accept();
}

void w_server::read_from_client(w_client *client, std::vector<uint8_t> data)
{
	client->handle_data(data);
}

void w_server::client_malformed_packet(w_client *client)
{
	//todo
	client->kick();
}

void w_server::client_disconnected(w_client *client)
{
	auto index = std::find(this->clients.begin(), this->clients.end(), client);
	if (index == this->clients.end())
		return;
	this->clients.erase(index);
	delete client;
}

void w_server::handle_client_packet(w_client *client, w_packet *packet)
{
	client->receive_packet(packet);
	this->game.lua_vm.receive_packet(client, packet);
	delete packet;
}
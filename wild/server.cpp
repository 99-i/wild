#include <plog/Log.h>
#include <chrono>
#include <algorithm>
#include <asio.hpp>
#include "server.h"
#include "client.h"
#include "packet.h"

constexpr int SERVER_BACKLOG = 128;

wild::server::server(int port) : port(port), game(this), acceptor(context, tcp::endpoint(tcp::v4(), port))
{
}

void wild::server::run()
{
	this->game_mutex.lock();
	this->running = true;
	PLOGI << "Running server on port " << port;

	this->accept_thread = std::thread([](wild::server *server)
		{
			server->start_accept();
			server->context.run();
		}, this);

	this->game_thread = std::thread(&game::start, &this->game);
	this->game_mutex.unlock();
}

void wild::server::restart()
{
	this->game_mutex.lock();
	auto start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	PLOGD << "Server restarted in " << duration_cast<std::chrono::milliseconds>(end - start);
	this->game_mutex.unlock();
}
void wild::server::start_accept()
{
	if (!this->running)
	{
		return;
	}
	tcp::socket *socket = new tcp::socket(this->context);
	this->acceptor.async_accept(*socket, std::bind(&wild::server::handle_accept, this, socket, std::placeholders::_1));
}

void wild::server::handle_accept(tcp::socket *socket, asio::error_code ec)
{
	PLOGD << "NUM CLIENTS: " << this->clients.size();
	if (!ec)
	{
		wild::client *client = new wild::client(socket, this);
		std::thread read_thread(&client::start_read, client);
		read_thread.detach();
		this->clients_mutex.lock();
		this->clients.push_back(client);
		this->clients_mutex.unlock();
	}
	this->start_accept();
}

void wild::server::read_from_client(wild::client *client, std::vector<uint8_t> data)
{
	client->handle_data(data);
}

void wild::server::client_malformed_packet(wild::client *client)
{
	//todo
	client->kick("Malformed Packet!");
}

void wild::server::client_disconnected(wild::client *client)
{
	this->clients_mutex.lock();
	auto index = std::find(this->clients.begin(), this->clients.end(), client);
	if (index == this->clients.end())
		return;
	this->clients.erase(index);
	this->clients_mutex.unlock();
	delete client;
}

void wild::server::handle_client_packet(wild::client *client, wild::packet *packet)
{
	client->receive_packet(packet);
	delete packet;
}
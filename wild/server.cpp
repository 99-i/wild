#include <plog/Log.h>
#include <chrono>
#include "server.h"
#include "client.h"
#include "packet.h"

constexpr int SERVER_BACKLOG = 128;

static void connection_cb(uv_stream_t *server, int status);
static void thread_cb(void *server);
static void allocate_buffer_cb(uv_handle_t *handle, size_t size, uv_buf_t *buffer);
static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer);

w_server::w_server(int port) : port(port), game(this)
{
	uv_loop_init(&this->loop);
}

void w_server::run()
{
	uv_tcp_init(&this->loop, &this->tcp_handle);

	sockaddr_in address;
	uv_ip4_addr("127.0.0.1", this->port, &address);
	uv_tcp_bind(&this->tcp_handle, reinterpret_cast<sockaddr *>(&address), 0);
	uv_listen(reinterpret_cast<uv_stream_t *>(&this->tcp_handle), SERVER_BACKLOG, connection_cb);

	PLOGI << "Running server on port " << port;

	this->game.start();
	this->game.lua_vm.start();
	this->run_thread = std::thread(thread_cb, this);
}

void w_server::restart()
{
	auto start = std::chrono::high_resolution_clock::now();
	this->game.lua_vm.reset();
	this->game.lua_vm.start();
	auto end = std::chrono::high_resolution_clock::now();
	PLOGD << "Server restarted in " << duration_cast<std::chrono::milliseconds>(end - start);
}

void w_server::on_connection(int status)
{
	w_client *client = new w_client(this);

	int r = uv_accept(reinterpret_cast<uv_stream_t *>(&this->tcp_handle), reinterpret_cast<uv_stream_t *>(&client->tcp_handle));

	if (r < 0)
	{
		PLOGF << "Encountered error while accepting client.";
		delete client;
		return;
	}

	this->clients.push_back(client);

	uv_read_start(reinterpret_cast<uv_stream_t *>(&client->tcp_handle), allocate_buffer_cb, read_cb);
}

void w_server::handle_read(w_client *client, ssize_t nread, const uv_buf_t *buffer)
{
	if (nread == UV_EOF)
	{
		client->disconnect = true;
		//this->clients.erase(std::remove(this->clients.begin(), this->clients.end(), client), this->clients.end());
		//delete client;
	} else if (nread >= 0)
	{
		if (buffer->len > 0)
			this->read_from_client(client, std::vector<uint8_t>(buffer->base, buffer->base + nread));
	}

	if (buffer->base)
	{
		delete buffer->base;
	}
}

void w_server::read_from_client(w_client *client, std::vector<uint8_t> data)
{
	client->handle_data(data);
}

void w_server::client_malformed_packet(w_client *client)
{
	//TODO: call lua.
	//todo: actually kick them.
	client->disconnect = true;
	//this->clients.erase(std::remove(this->clients.begin(), this->clients.end(), client), this->clients.end());
	//delete client;
}

void w_server::handle_client_packet(w_client *client, w_packet packet)
{
	this->game.lua_vm.receive_packet(client, packet);
}

void w_server::loop_start()
{
	uv_run(&this->loop, UV_RUN_DEFAULT);
}

static void connection_cb(uv_stream_t *server, int status)
{
	reinterpret_cast<w_server *>(reinterpret_cast<char *>(server) - offsetof(w_server, tcp_handle))->on_connection(status);
}
static void thread_cb(void *server)
{
	reinterpret_cast<w_server *>(server)->loop_start();
}
static void allocate_buffer_cb(uv_handle_t *handle, size_t size, uv_buf_t *buffer)
{
	buffer->base = new char[size];
	buffer->len = size;
}
static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer)
{
	w_client *client = reinterpret_cast<w_client *>(reinterpret_cast<char *>(stream) - offsetof(w_client, tcp_handle));
	client->server->handle_read(client, nread, buffer);
}
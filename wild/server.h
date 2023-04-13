#pragma once
#include <uv.h>
#include <vector>
#include <thread>

#include "game.h"

struct w_client;
struct w_packet;

struct w_server
{
	bool running = false;
	int port;

	uv_loop_t loop;
	uv_tcp_t tcp_handle;
	std::thread run_thread;
	w_game game;

	void on_connection(int status);
	void loop_start();
	void handle_read(w_client *client, ssize_t nread, const uv_buf_t *buffer);

	void read_from_client(w_client *client, std::vector<uint8_t> data);

	//called when a client sends malformed data
	void client_malformed_packet(w_client *client);

	void handle_client_packet(w_client *client, w_packet packet);

	std::vector<w_client *> clients;

	w_server(int port = 25565);
	void run();

	void restart();
};

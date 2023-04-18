#include <winsock2.h>
#include <string>
#include <iostream>
#include <plog/Initializers/ConsoleInitializer.h>
#include <chrono>
#include <random>
#include "random.h"
#include "console.h"
#include "packet.h"
#include "server.h"
#include "main.h"

console c;

int main(int argc, char **argv)
{
	static WildAppender appender;
	plog::init(plog::verbose, &appender);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		PLOGF << "Failed to initialize WinSock2.2";
		PLOGF << "Exiting...";
		WSACleanup();
		exit(1);
	}

	w_server *server = new w_server();
	c.server = server;
	server->run();
	c.start();

	WSACleanup();
}
#include <winsock2.h>
#include <string>
#include <iostream>
#include <plog/Initializers/ConsoleInitializer.h>
#include <chrono>
#include "console.h"
#include "packet.h"
#include "server.h"
#include "main.h"

console c;

int main(int argc, char **argv)
{
	static WildAppender appender;
	plog::init(plog::verbose, &appender);

	w_server *server = new w_server();
	c.server = server;
	server->run();
	c.start();
}
#include "console.h"
#include "main.h"

int main(int argc, char **argv)
{
	wild::console c;
	wild::logging::c = &c;
	static wild::logging::WildAppender appender;
	plog::init(plog::verbose, &appender);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		PLOGF << "Failed to initialize WinSock2.2";
		PLOGF << "Exiting...";
		WSACleanup();
		return 1;
	}
	wild::server *server = new wild::server();
	c.server = server;
	server->run();
	c.start();

	WSACleanup();
}
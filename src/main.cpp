#include "interface/console.hpp"
#include "interface/logger.hpp"
#include "net/server.hpp"

using namespace wild;
int main(int argc, char **argv)
{
	srand(time(NULL));

	console c;
	logging::c = &c;
	static logging::WildAppender appender;
	plog::init(plog::verbose, &appender);

	wild::server *server = new wild::server();
	c.server = server;
	server->run();
	c.start();
}

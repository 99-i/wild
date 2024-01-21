#include <sdkddkver.h>
#include <iostream>
#include <asio.hpp>
#include "net/server.hpp"

static bool port_in_use(unsigned short port) {
	using namespace asio;
	using ip::tcp;

	io_service svc;
	tcp::acceptor a(svc);

	asio::error_code ec;
	a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), port }, ec);

	return ec == error::address_in_use;
}

constexpr uint16_t PORT = 25565;

int main()
{
	if (port_in_use(PORT))
	{
		std::cerr << "PORT " << PORT << " already in use. Test inconclusive.";
		return 1;
	}

	wild::server* server = new wild::server(PORT);

	server->run();

	return !port_in_use(PORT);
}
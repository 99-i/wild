#include "client.h"
#include "player.h"

wild::player::player(wild::client &client) : _client(client)
{
}

const std::string &wild::player::username() const
{
	return this->_username;
}
const wild::client &wild::player::client() const
{
	return this->_client;
}
#include <uv.h>
#include <cassert>
#include "server.h"
#include "client.h"
#include "packet.h"
#include "clientbound_packet.h"

w_client::w_client(w_server *server) : server(server)
{
	uv_tcp_init(&this->server->loop, &this->tcp_handle);
	this->read_stream = new w_packet_read_stream(this);
}

void w_client::disconnect_client()
{
	this->disconnect = true;
	uv_close(reinterpret_cast<uv_handle_t *>(&this->tcp_handle), NULL);
}

void w_client::handle_data(const std::vector<uint8_t> &data)
{
	this->read_stream->handle_data(data);
}

void w_client::reset_reader()
{
	delete this->read_stream;
	this->read_stream = new w_packet_read_stream(this);
}

struct write_context
{
	uv_write_t req;
	uv_buf_t bufs[3];
};

void on_write_end(uv_write_t *req, int status)
{
	write_context *context = reinterpret_cast<write_context *>(req);
	delete context;
}

void w_client::send_packet(lua_clientbound_packet *packet)
{
	//todo
	std::vector<uint8_t> id_varint = write_fn::write_varint(packet->id);
	std::vector<uint8_t> length_varint = write_fn::write_varint(id_varint.size() + packet->data.size());

	write_context *context = new write_context();

	context->bufs[0].base = (char *)length_varint.data();
	context->bufs[0].len = length_varint.size();

	context->bufs[1].base = (char *)id_varint.data();
	context->bufs[1].len = id_varint.size();

	context->bufs[2].base = (char *)packet->data.data();
	context->bufs[2].len = packet->data.size();

	int written = uv_write(&context->req, reinterpret_cast<uv_stream_t *>(&this->tcp_handle), context->bufs, 3, on_write_end);
}

std::string w_client::get_state_str()
{
	switch (this->state)
	{
		case client_state::HANDSHAKING:
			return "handshaking";
			break;
		case client_state::STATUS:
			return "status";
			break;
		case client_state::PLAY:
			return "play";
			break;
		case client_state::LOGIN:
			return "login";
			break;
	}
}
void w_client::set_state_str(std::string str)
{
	if (str == "handshaking")
		this->state = client_state::HANDSHAKING;
	else if (str == "status")
		this->state = client_state::STATUS;
	else if (str == "play")
		this->state = client_state::PLAY;
	else if (str == "login")
		this->state = client_state::LOGIN;
}

w_client::~w_client()
{
	uv_close(reinterpret_cast<uv_handle_t *>(&this->tcp_handle), NULL);
}
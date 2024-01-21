#pragma once
#include "asio.hpp"
#include "packet/packet.hpp"
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

using asio::ip::tcp;
namespace wild
{
class server;
class player;
class clientbound_packet;
class packet;

// a client connected to the server.
class client
{
	// the size of the buffer that receives raw tcp stream data.
	static constexpr size_t READ_BUFFER_SIZE = 512;
	client_state state = client_state::HANDSHAKING;
	wild::server &server;

#pragma region TCP

	// buffer that receives raw tcp stream data.
	uint8_t read_buf[READ_BUFFER_SIZE];

	tcp::socket socket;

	// internally handle an array of bytes of data
	bool handle_data(const std::vector<uint8_t> &data);

	// mutex for write_queue
	std::mutex write_queue_mutex;
	// if async_write was called and is still not done. this whole write queue
	// stuff is b/c asio doesn't let you call async_write while another
	// async_write is in progress.
	bool write_in_progress = false;
	// a deque of data that is in queue to be sent.
	std::deque<std::vector<uint8_t>> write_queue;

	// called when asio sent data to the client
	void handle_write(asio::error_code ec);

#pragma endregion

#pragma region PacketParser
	// packet parsing works like this: first read_state is set to
	// read_length_varint. when the length varint is completed reading, then we
	// wait for all of the data to be sent to the client, before reading the
	// whole packet at once. after we are done reading a complete packet, we
	// call reset_reader()

	enum
	{
		// in the middle of reading the length varint
		READ_LENGTH_VARINT,
		// waiting for all the data to be sent so we can read it.
		WAITING_FOR_ALL_DATA
	} read_state = READ_LENGTH_VARINT;

	// reads the length of the packet
	wild::read_fn::varint_reader packet_length_reader;

	// goes down every time a byte is read.
	uint32_t packet_length_remaining = 0;

	// the buffer of data that gets written to while we're waiting for the whole
	// thing to be finished sending.
	std::vector<uint8_t> packet_buffer;

	// reset the packet reader.
	void reset_reader();

	// return true if data was handled correctly.
	// return false if malformed data from client.
	bool read_packet(std::vector<uint8_t> data);

#pragma endregion

#pragma region KeepAlive
	// the id that the server expects within 20s for the client to send back.
	int32_t keepalive_id;

	// kick if > 20s
	std::chrono::high_resolution_clock::time_point last_time_since_keepalive =
		std::chrono::high_resolution_clock::now();

	// changes keepalive_id and sends the keepalive packet with the updated
	// keepalive_id
	void do_keepalive();

	// the runnable that calls do_keepalive() every 10 seconds.
	std::optional<uint32_t> keepalive_runnable_id = std::nullopt;

	// create and start the keepalive runnable.
	void start_keepalive();

#pragma endregion

	// these handle all the different packets that the client sends.
	// more info: https://wiki.vg/index.php?title=Protocol&oldid=6003
#pragma region PacketHandleFunctions
	void Handle_HANDSHAKING_Handshake(const wild::packet &packet);

	void Handle_STATUS_Request(const wild::packet &packet);
	void Handle_STATUS_Ping(const wild::packet &packet);

	void Handle_LOGIN_LoginStart(const wild::packet &packet);
	void Handle_LOGIN_EncryptionResponse(const wild::packet &packet);

	void Handle_PLAY_KeepAlive(const wild::packet &packet);
	void Handle_PLAY_ChatMessage(const wild::packet &packet);
	void Handle_PLAY_UseEntity(const wild::packet &packet);
	void Handle_PLAY_Player(const wild::packet &packet);
	void Handle_PLAY_PlayerPosition(const wild::packet &packet);
	void Handle_PLAY_PlayerLook(const wild::packet &packet);
	void Handle_PLAY_PlayerPositionAndLook(const wild::packet &packet);
	void Handle_PLAY_PlayerDigging(const wild::packet &packet);
	void Handle_PLAY_PlayerBlockPlacement(const wild::packet &packet);
	void Handle_PLAY_HeldItemChange(const wild::packet &packet);
	void Handle_PLAY_Animation(const wild::packet &packet);
	void Handle_PLAY_EntityAction(const wild::packet &packet);
	void Handle_PLAY_SteerVehicle(const wild::packet &packet);
	void Handle_PLAY_CloseWindow(const wild::packet &packet);
	void Handle_PLAY_ClickWindow(const wild::packet &packet);
	void Handle_PLAY_ConfirmTransaction(const wild::packet &packet);
	void Handle_PLAY_CreativeInventoryAction(const wild::packet &packet);
	void Handle_PLAY_EnchantItem(const wild::packet &packet);
	void Handle_PLAY_UpdateSign(const wild::packet &packet);
	void Handle_PLAY_PlayerAbilities(const wild::packet &packet);
	void Handle_PLAY_TabComplete(const wild::packet &packet);
	void Handle_PLAY_ClientSettings(const wild::packet &packet);
	void Handle_PLAY_ClientStatus(const wild::packet &packet);
	void Handle_PLAY_PluginMessage(const wild::packet &packet);
#pragma endregion

  public:
	// the player bound to the client. this is a ptr because player only gets
	// created when the client actually joins the game
	wild::player *player = nullptr;
	// kick the client if they haven't sent a keepalive packet in 20 seconds.
	void kick_if_keepalive_expired();

	// start reading from TCP stream
	void start_read();
	// send raw data to TCP stream
	void send_data(const std::vector<uint8_t> &data);
	// send a packet
	void send_packet(const wild::clientbound_packet &packet);
	// send a packet and call a callback after the packet was finished sending
	void send_packet(const wild::clientbound_packet &packet,
					 std::function<void(wild::client *)> callback);
	// send multiple packets
	void send_packets(const std::vector<wild::clientbound_packet> &packets);

	// received a packet.
	void receive_packet(const wild::packet &packet);

	client(tcp::socket &&socket, wild::server &server);
	~client();

	// kick the client for a reason
	void kick(std::string reason);
};
} // namespace wild

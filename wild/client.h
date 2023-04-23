#pragma once
#include <asio.hpp>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "packet.h"

using asio::ip::tcp;
namespace wild
{
	struct server;
	struct player;
	struct clientbound_packet;
	struct packet;

	struct client
	{
		static constexpr size_t READ_BUFFER_SIZE = 512;
		client_state state = client_state::HANDSHAKING;
		wild::server *server;

		wild::player *player = nullptr;

		bool handle_data(const std::vector<uint8_t> &data);

		void reset_reader();

		std::mutex write_queue_mutex;
		bool write_in_progress = false;
		std::deque<std::vector<uint8_t>> write_queue;

		void handle_write(asio::error_code ec);

		void send_packet(const wild::clientbound_packet *packet);
		void send_packet(const wild::clientbound_packet *packet, std::function<void(wild::client *)> callback);
		enum
		{
			//in the middle of reading the length varint
			READ_LENGTH_VARINT,
			//waiting for all the data to be sent so we can read it.
			WAITING_FOR_ALL_DATA
		} read_state = BEGIN;

#pragma region PacketParserFunctions

		//reads the length of the packet
		wild::read_fn::varint_reader packet_length_reader;

		//goes down every time a byte is read. if 0, throw error.
		uint32_t packet_length_remaining = 0;

		//the buffer of data that gets written to while we're waiting for the whole thing to be finished sending.
		std::vector<uint8_t> packet_buffer;

		//return true if data was handled correctly.
		//return false if malformed data from client.
		bool read_packet(std::vector<uint8_t> data);

#pragma endregion
		void receive_packet(const wild::packet *packet);

		uint8_t read_buf[READ_BUFFER_SIZE];
		void start_read();

		tcp::socket *socket;

		bool disconnect = false;

		client(tcp::socket *socket, wild::server *server);
		~client();

		int32_t keepalive_id;

		//kick if > 20s
		std::chrono::high_resolution_clock::time_point last_time_since_keepalive = std::chrono::high_resolution_clock::now();

		void do_keepalive();

		uint32_t keepalive_runnable_id = 0;
		void start_keepalive();

		void kick(std::string reason);

#pragma region PacketHandleFunctions
		void Handle_HANDSHAKING_Handshake(const wild::packet *packet);
		void Handle_STATUS_Request(const wild::packet *packet);
		void Handle_STATUS_Ping(const wild::packet *packet);
		void Handle_LOGIN_LoginStart(const wild::packet *packet);
		void Handle_LOGIN_EncryptionResponse(const wild::packet *packet);
		void Handle_PLAY_KeepAlive(const wild::packet *packet);
		void Handle_PLAY_ChatMessage(const wild::packet *packet);
		void Handle_PLAY_UseEntity(const wild::packet *packet);
		void Handle_PLAY_Player(const wild::packet *packet);
		void Handle_PLAY_PlayerPosition(const wild::packet *packet);
		void Handle_PLAY_PlayerLook(const wild::packet *packet);
		void Handle_PLAY_PlayerPositionAndLook(const wild::packet *packet);
		void Handle_PLAY_PlayerDigging(const wild::packet *packet);
		void Handle_PLAY_PlayerBlockPlacement(const wild::packet *packet);
		void Handle_PLAY_HeldItemChange(const wild::packet *packet);
		void Handle_PLAY_Animation(const wild::packet *packet);
		void Handle_PLAY_EntityAction(const wild::packet *packet);
		void Handle_PLAY_SteerVehicle(const wild::packet *packet);
		void Handle_PLAY_CloseWindow(const wild::packet *packet);
		void Handle_PLAY_ClickWindow(const wild::packet *packet);
		void Handle_PLAY_ConfirmTransaction(const wild::packet *packet);
		void Handle_PLAY_CreativeInventoryAction(const wild::packet *packet);
		void Handle_PLAY_EnchantItem(const wild::packet *packet);
		void Handle_PLAY_UpdateSign(const wild::packet *packet);
		void Handle_PLAY_PlayerAbilities(const wild::packet *packet);
		void Handle_PLAY_TabComplete(const wild::packet *packet);
		void Handle_PLAY_ClientSettings(const wild::packet *packet);
		void Handle_PLAY_ClientStatus(const wild::packet *packet);
		void Handle_PLAY_PluginMessage(const wild::packet *packet);
#pragma endregion
	};
}

#pragma once
#include <asio.hpp>
#include <vector>
#include <deque>
#include <sol/sol.hpp>
#include <map>
#include <functional>

#include "player.h"

struct w_server;
struct w_packet_read_stream;
struct w_clientbound_packet;
struct w_packet;
struct w_client;

using asio::ip::tcp;

struct w_client
{
	static constexpr size_t READ_BUFFER_SIZE = 10000;
	enum class client_state
	{
		HANDSHAKING,
		STATUS,
		LOGIN,
		PLAY
	} state = client_state::HANDSHAKING;

	w_server *server;

	w_player player;

	w_packet_read_stream *read_stream;
	bool handle_data(const std::vector<uint8_t> &data);

	void reset_reader();

	std::mutex write_queue_mutex;
	bool write_in_progress = false;
	std::deque<std::vector<uint8_t>> write_queue;

	void handle_write(asio::error_code ec);

	void send_packet(const w_clientbound_packet *packet);
	void send_packet(const w_clientbound_packet *packet, std::function<void(w_client *)> callback);
	void receive_packet(const w_packet *packet);

	uint8_t read_buf[READ_BUFFER_SIZE];
	void start_read();

	tcp::socket *socket;

	bool disconnect = false;

	w_client(tcp::socket *socket, w_server *server);
	~w_client();

	uint32_t keepalive_id;
	uint32_t keepalive_runnable_id;
	void do_keepalive(uint32_t runnable_id);

	void kick();

#pragma region PacketHandleFunctions
	void Handle_HANDSHAKING_Handshake(const w_packet *packet);
	void Handle_STATUS_Request(const w_packet *packet);
	void Handle_STATUS_Ping(const w_packet *packet);
	void Handle_LOGIN_LoginStart(const w_packet *packet);
	void Handle_LOGIN_EncryptionResponse(const w_packet *packet);
	void Handle_PLAY_KeepAlive(const w_packet *packet);
	void Handle_PLAY_ChatMessage(const w_packet *packet);
	void Handle_PLAY_UseEntity(const w_packet *packet);
	void Handle_PLAY_Player(const w_packet *packet);
	void Handle_PLAY_PlayerPosition(const w_packet *packet);
	void Handle_PLAY_PlayerLook(const w_packet *packet);
	void Handle_PLAY_PlayerPositionAndLook(const w_packet *packet);
	void Handle_PLAY_PlayerDigging(const w_packet *packet);
	void Handle_PLAY_PlayerBlockPlacement(const w_packet *packet);
	void Handle_PLAY_HeldItemChange(const w_packet *packet);
	void Handle_PLAY_Animation(const w_packet *packet);
	void Handle_PLAY_EntityAction(const w_packet *packet);
	void Handle_PLAY_SteerVehicle(const w_packet *packet);
	void Handle_PLAY_CloseWindow(const w_packet *packet);
	void Handle_PLAY_ClickWindow(const w_packet *packet);
	void Handle_PLAY_ConfirmTransaction(const w_packet *packet);
	void Handle_PLAY_CreativeInventoryAction(const w_packet *packet);
	void Handle_PLAY_EnchantItem(const w_packet *packet);
	void Handle_PLAY_UpdateSign(const w_packet *packet);
	void Handle_PLAY_PlayerAbilities(const w_packet *packet);
	void Handle_PLAY_TabComplete(const w_packet *packet);
	void Handle_PLAY_ClientSettings(const w_packet *packet);
	void Handle_PLAY_ClientStatus(const w_packet *packet);
	void Handle_PLAY_PluginMessage(const w_packet *packet);
#pragma endregion

	void spawn_player_in();
	void send_chunk_updates();

	void do_keepalive();
};

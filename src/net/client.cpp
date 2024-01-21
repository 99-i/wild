#include "net/client.hpp"
#include "common.hpp"
#include "game/entity/player.hpp"
#include "net/packet/clientbound_packet.hpp"
#include "net/packet/packet.hpp"
#include "net/server.hpp"
#include "util/random.hpp"
#include <map>
#include <nlohmann/json.hpp>
#include <plog/Helpers/HexDump.h>
#include <plog/Log.h>
#include <zlib.h>

namespace wild
{
	typedef void (client::*packet_callback)(const packet &packet);

	client::client(asio::ip::tcp::socket &&socket, wild::server &server)
		: server(server), socket(std::move(socket))
	{
	}

	bool client::handle_data(const std::vector<uint8_t> &data)
	{
		for (uint8_t byte : data)
			{
				switch (this->read_state)
					{
						// this is a byte of the length varint.
						case READ_LENGTH_VARINT: {
							if (this->packet_length_reader.push_byte(byte))
								{
									this->read_state = WAITING_FOR_ALL_DATA;
									this->packet_length_remaining =
										this->packet_length_reader.value;
								}
							break;
						}
						case WAITING_FOR_ALL_DATA: {
							this->packet_length_remaining--;
							this->packet_buffer.push_back(byte);
							if (this->packet_length_remaining <= 0)
								{
									if (!this->read_packet(this->packet_buffer))
										return false;
									this->reset_reader();
								}
						}
					}
			}
		return true;
	}

	// https://wiki.vg/index.php?title=Protocol&oldid=6003
	bool client::read_packet(std::vector<uint8_t> data)
	{
		auto needle = this->packet_buffer.begin();
		auto end = this->packet_buffer.end();
		std::optional<int32_t> id = read_fn::read_varint(needle, end);
		if (!id.has_value())
			return false;

		const packet_form *form = nullptr;
		bool found_form = false;
		for (int i = 0; i < FORMS_SIZE; i++)
			{
				form = &forms[i];
				if (form->id == id && this->state == form->state)
					{
						found_form = true;
						break;
					}
			}

		// todo: finish this.
		// also there may be a better way to do this.

		packet packet;
		packet.name = form->name;
		packet.id = id.value();

#define KICK_FOR_MALFORMED                                                     \
	do                                                                         \
		{                                                                      \
			this->server.client_malformed_packet(this);                        \
			PLOGD << "Kick for malformed.";                                    \
			return false;                                                      \
	} while (false);

#define READ_SHORT(name)                                                       \
	auto name = read_fn::read_i16(needle, end);                                \
	if (!name.has_value())                                                     \
		{                                                                      \
			KICK_FOR_MALFORMED                                                 \
		}
#define READ_BYTE_ARRAY(size, name)                                            \
	auto name = read_fn::read_byte_array(size, needle, end);                   \
	if (!name.has_value())                                                     \
		{                                                                      \
			KICK_FOR_MALFORMED                                                 \
		}

		if (!found_form)
			{
				switch (this->state)
					{
					case client_state::HANDSHAKING:
						break;
					case client_state::STATUS:
						break;
					case client_state::LOGIN:
						switch (id.value())
							{
							case 0x01:
								// encryption response.
								READ_SHORT(shared_secret_length)
								packet.data.emplace(
									"shared_secret_length",
									shared_secret_length.value());

								READ_BYTE_ARRAY(shared_secret_length.value(),
												shared_secret)
								packet.data.emplace("shared_secret",
													shared_secret.value());

								READ_SHORT(verify_token_length)
								packet.data.emplace(
									"verify_token_length",
									verify_token_length.value());

								READ_BYTE_ARRAY(verify_token_length.value(),
												verify_token)
								packet.data.emplace("verify_token",
													verify_token.value());

								packet.name = "encryption_response";
								this->server.handle_client_packet(this, packet);

								return true;
							}
						break;
					case client_state::PLAY:
						break;
					}
				return true;
			}

		for (int i = 0; i < form->num_fields; i++)
			{
				packet_form::field field = form->fields[i];
				if (needle == end)
					{
						KICK_FOR_MALFORMED
					}
				switch (field.type)
					{
						// todo: put this in a macro. may make code less
						// readable though.
						case data_type::BOOL: {
							auto res = read_fn::read_bool(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::BYTE: {
							auto res = read_fn::read_i8(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::UNSIGNED_BYTE: {
							auto res = read_fn::read_u8(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::SHORT: {
							auto res = read_fn::read_i16(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::UNSIGNED_SHORT: {
							auto res = read_fn::read_u16(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::INT: {
							auto res = read_fn::read_i32(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::LONG: {
							auto res = read_fn::read_i64(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::FLOAT: {
							auto res = read_fn::read_float(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::DOUBLE: {
							auto res = read_fn::read_double(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::STRING: {
							auto res = read_fn::read_string(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
						case data_type::VARINT: {
							auto res = read_fn::read_varint(needle, end);
							if (!res.has_value())
								{
									KICK_FOR_MALFORMED
								}

							packet.data.emplace(field.name, res.value());
						}
						break;
					default:
						break;
					}
			}

		this->server.handle_client_packet(this, packet);
		return true;
	}

	void client::reset_reader()
	{
		this->read_state = READ_LENGTH_VARINT;
		this->packet_length_reader.reset();
		this->packet_length_remaining = 0;
		this->packet_buffer.clear();
	}

	void client::start_read()
	{
		bool dc = false;
		while (!dc)
			{
				try
					{
						size_t size = this->socket.read_some(
							asio::buffer(this->read_buf, READ_BUFFER_SIZE));
						std::vector<uint8_t> data(this->read_buf,
												  this->read_buf + size);
						dc = !this->handle_data(data);
					}
				catch (asio::system_error ec)
					{
						dc = true;
					}
			}
		this->server.client_disconnected(this);
	}

	void client::handle_write(asio::error_code ec)
	{
		if (ec)
			{
				PLOGF << "Error writing: " << ec.message();
				this->server.client_disconnected(this);
				return;
			}

		this->write_queue_mutex.lock();
		this->write_in_progress = false;
		if (!this->write_queue.empty())
			{
				this->write_in_progress = true;
				asio::async_write(
					this->socket,
					asio::buffer(this->write_queue.front().data(),
								 this->write_queue.front().size()),
					std::bind(&client::handle_write, this,
							  std::placeholders::_1));
				this->write_queue.pop_front();
			}
		this->write_queue_mutex.unlock();
	}

	void client::send_data(const std::vector<uint8_t> &data)
	{
		this->write_queue_mutex.lock();
		this->write_queue.push_back(data);
		if (this->write_queue.size() <= 1)
			{
				this->write_in_progress = true;
				asio::async_write(
					this->socket,
					asio::buffer(this->write_queue.front().data(),
								 this->write_queue.front().size()),
					std::bind(&client::handle_write, this,
							  std::placeholders::_1));
				this->write_queue.pop_front();
			}
		this->write_queue_mutex.unlock();
	}
	void client::send_packet(const clientbound_packet &packet)
	{
		this->write_queue_mutex.lock();
		// todo
		std::vector<uint8_t> data = packet.package();
		this->write_queue.push_back(data);
		if (this->write_queue.size() <= 1)
			{
				this->write_in_progress = true;
				asio::async_write(
					this->socket,
					asio::buffer(this->write_queue.front().data(),
								 this->write_queue.front().size()),
					std::bind(&client::handle_write, this,
							  std::placeholders::_1));
				this->write_queue.pop_front();
			}
		this->write_queue_mutex.unlock();
	}

	void client::send_packet(const clientbound_packet &packet,
							 std::function<void(client *)> callback)
	{
		this->write_queue_mutex.lock();
		std::vector<uint8_t> data = packet.package();
		this->write_queue.push_back(data);
		if (this->write_queue.size() <= 1)
			{
				this->write_in_progress = true;
				asio::async_write(
					this->socket,
					asio::buffer(this->write_queue.front().data(),
								 this->write_queue.front().size()),
					[this, callback](asio::error_code ec, int i) {
						this->handle_write(ec);
						callback(this);
					});
				this->write_queue.pop_front();
			}
		this->write_queue_mutex.unlock();
	}

	void client::send_packets(const std::vector<clientbound_packet> &packets)
	{
		// probably a better way to do this lol.
		for (auto &packet : packets)
			{
				this->send_packet(packet);
			}
	}

	void client::receive_packet(const packet &packet)
	{
		static std::map<std::tuple<client_state, uint8_t>, packet_callback>
			callback_map = {
				{{client_state::HANDSHAKING, 0x00},
				 &client::Handle_HANDSHAKING_Handshake},
				{{client_state::STATUS, 0x00}, &client::Handle_STATUS_Request},
				{{client_state::STATUS, 0x01}, &client::Handle_STATUS_Ping},
				{{client_state::LOGIN, 0x00}, &client::Handle_LOGIN_LoginStart},
				{{client_state::LOGIN, 0x01},
				 &client::Handle_LOGIN_EncryptionResponse},
				{{client_state::PLAY, 0x00}, &client::Handle_PLAY_KeepAlive},
				{{client_state::PLAY, 0x01}, &client::Handle_PLAY_ChatMessage},
				{{client_state::PLAY, 0x02}, &client::Handle_PLAY_UseEntity},
				{{client_state::PLAY, 0x03}, &client::Handle_PLAY_Player},
				{{client_state::PLAY, 0x04},
				 &client::Handle_PLAY_PlayerPosition},
				{{client_state::PLAY, 0x05}, &client::Handle_PLAY_PlayerLook},
				{{client_state::PLAY, 0x06},
				 &client::Handle_PLAY_PlayerPositionAndLook},
				{{client_state::PLAY, 0x07},
				 &client::Handle_PLAY_PlayerDigging},
				{{client_state::PLAY, 0x09},
				 &client::Handle_PLAY_HeldItemChange},
				{{client_state::PLAY, 0x0A}, &client::Handle_PLAY_Animation},
				{{client_state::PLAY, 0x0B}, &client::Handle_PLAY_EntityAction},
				{{client_state::PLAY, 0x0C}, &client::Handle_PLAY_SteerVehicle},
				{{client_state::PLAY, 0x0D}, &client::Handle_PLAY_CloseWindow},
				{{client_state::PLAY, 0x0F},
				 &client::Handle_PLAY_ConfirmTransaction},
				{{client_state::PLAY, 0x11}, &client::Handle_PLAY_EnchantItem},
				{{client_state::PLAY, 0x12}, &client::Handle_PLAY_UpdateSign},
				{{client_state::PLAY, 0x13},
				 &client::Handle_PLAY_PlayerAbilities},
				{{client_state::PLAY, 0x14}, &client::Handle_PLAY_TabComplete},
				{{client_state::PLAY, 0x15},
				 &client::Handle_PLAY_ClientSettings},
				{{client_state::PLAY, 0x16}, &client::Handle_PLAY_ClientStatus},
			};

		// is this inefficient ?
		(this->*(callback_map[std::make_tuple(this->state, packet.id)]))(
			packet);
	}

	void client::do_keepalive()
	{
		this->keepalive_id = Random::random_i32();
		auto keepalive =
			packet_builder(0x00).append_i32(this->keepalive_id).build();
		this->send_packet(keepalive);
	}

	void client::kick_if_keepalive_expired()
	{
		using namespace std::chrono_literals;
		if ((std::chrono::high_resolution_clock::now() -
			 this->last_time_since_keepalive) > 20s)
			{
				// todo: get strings language file
				this->kick("No KeepAlive ID!");
			}
	}

	void client::start_keepalive()
	{
		this->keepalive_runnable_id = this->server.game.create_c_runnable(
			[this](uint32_t runnable_id) {
				this->do_keepalive();
				return true;
			},
			std::make_tuple(runnable::run_type::TIMER, 20 * 10, 20 * 10));
	}

	void client::kick(std::string reason)
	{
		nlohmann::json kick_reason = {{"text", reason}};

		auto disconnect =
			packet_builder(0x40).append_string(kick_reason.dump()).build();

		this->send_packet(disconnect, [](client *client) {
			client->server.client_disconnected(client);
		});
	}

	client::~client()
	{
		if (this->keepalive_runnable_id.has_value())
			this->server.game.stop_runnable(
				this->keepalive_runnable_id.value());
		this->socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		this->socket.cancel();
		this->socket.close();
	}

	void client::Handle_HANDSHAKING_Handshake(const packet &packet)
	{
		//(5 as of 1.7.10)
		int32_t protocol_version =
			std::get<int32_t>(packet.data.at("protocol_version"));
		// localhost
		std::string server_address =
			std::get<std::string>(packet.data.at("server_address"));
		// 25565
		uint16_t server_port =
			std::get<uint16_t>(packet.data.at("server_port"));
		// 1 for status, 2 for login
		int32_t next_state = std::get<int32_t>(packet.data.at("next_state"));

		if (next_state == 1)
			{
				this->state = client_state::STATUS;
			}
		else if (next_state == 2)
			{
				this->state = client_state::LOGIN;
			}
	}

	void client::Handle_STATUS_Request(const packet &packet)
	{
		nlohmann::json response = {
			// todo
			{"version", {{"name", "wild:1.7.10#5"}, {"protocol", 5}}},
			{"players",
			 {{"max", this->server.max_players},
			  {"online", this->server.game.overworld->get_players().size()},
			  {"sample", {}}}},
			{"description", {{"text", "Hello World"}}}};

		auto response_packet =
			packet_builder(0x00).append_string(response.dump()).build();
		this->send_packet(response_packet);
	}
	void client::Handle_STATUS_Ping(const packet &packet)
	{
		int64_t time = std::get<int64_t>(packet.data.at("time"));

		auto pong = packet_builder(0x01).append_i64(time).build();
		this->send_packet(pong);
	}

	void client::Handle_LOGIN_LoginStart(const packet &packet)
	{
		std::string name = std::get<std::string>(packet.data.at("name"));
		// todo: login

		auto encryption_request =
			packet_builder(0x01)
				.append_string("")
				.append_i16(this->server.keypair.asn1.size())
				.append_bytes(this->server.keypair.asn1)
				.append_i16(4)
				.append_i32(42)
				.build();

		this->send_packet(encryption_request);

		// auto login_success =
		//	packet_builder(0x02)
		//		.append_string("a75760cd-3c12-4b4c-829a-371aacb4c56a")
		//		.append_string(name)
		//		.build();

		// this->send_packet(login_success);
		// this->state = client_state::PLAY;

		//// todo: spawn player in
		// this->player = new player(*this);
		// this->player->_username = name;
		// game_event join_game_event =
		//	game_event::player_join(this->player);
		// this->server.game.queue_event(join_game_event);

		// this->start_keepalive();
	}

	void client::Handle_LOGIN_EncryptionResponse(const packet &packet)
	{
		PLOGD << "Got encryption request.";
	}

	void client::Handle_PLAY_KeepAlive(const packet &packet)
	{
		int32_t client_keepalive_id =
			std::get<int32_t>(packet.data.at("keep_alive_id"));
		if (this->keepalive_id != client_keepalive_id)
			{
				this->kick("Incorrect KeepAlive ID!");
			}
		this->last_time_since_keepalive =
			std::chrono::high_resolution_clock::now();
	}
	void client::Handle_PLAY_ChatMessage(const packet &packet)
	{
		std::string message = std::get<std::string>(packet.data.at("message"));
	}
	void client::Handle_PLAY_UseEntity(const packet &packet)
	{
		int32_t target = std::get<int32_t>(packet.data.at("target"));
		// 0 = Right-click, 1 = Left-click
		int8_t mouse = std::get<int8_t>(packet.data.at("mouse"));
	}
	void client::Handle_PLAY_Player(const packet &packet)
	{
		// True if the client is on the ground, False otherwise
		bool on_ground = std::get<bool>(packet.data.at("on_ground"));
	}
	void client::Handle_PLAY_PlayerPosition(const packet &packet)
	{
		// Absolute position
		double x = std::get<double>(packet.data.at("x"));
		// Absolute feet position, normally HeadY - 1.62. Used to modify the
		// players bounding box when going up stairs, crouching, etc…
		double feety = std::get<double>(packet.data.at("feety"));
		// Absolute head position
		double heady = std::get<double>(packet.data.at("heady"));
		// Absolute position
		double z = std::get<double>(packet.data.at("z"));
		// True if the client is on the ground, False otherwise
		bool on_ground = std::get<bool>(packet.data.at("on_ground"));

		game_event move_event =
			game_event::player_move(this->player, {x, feety, z});
		this->server.game.queue_event(move_event);
	}
	void client::Handle_PLAY_PlayerLook(const packet &packet)
	{
		// Absolute rotation on the X Axis, in degrees
		float yaw = std::get<float>(packet.data.at("yaw"));
		// Absolute rotation on the Y Axis, in degrees
		float pitch = std::get<float>(packet.data.at("pitch"));
		// True if the client is on the ground, False otherwise
		bool on_ground = std::get<bool>(packet.data.at("on_ground"));

		game_event look_event =
			game_event::player_look(this->player, yaw, pitch);
		this->server.game.queue_event(look_event);
	}
	void client::Handle_PLAY_PlayerPositionAndLook(const packet &packet)
	{
		// Absolute position
		double x = std::get<double>(packet.data.at("x"));
		// Absolute feet position. Is normally HeadY - 1.62. Used to modify the
		// players bounding box when going up stairs, crouching, etc…
		double feety = std::get<double>(packet.data.at("feety"));
		// Absolute head position
		double heady = std::get<double>(packet.data.at("heady"));
		// Absolute position
		double z = std::get<double>(packet.data.at("z"));
		// Absolute rotation on the X Axis, in degrees
		float yaw = std::get<float>(packet.data.at("yaw"));
		// Absolute rotation on the Y Axis, in degrees
		float pitch = std::get<float>(packet.data.at("pitch"));
		// True if the client is on the ground, False otherwise
		bool on_ground = std::get<bool>(packet.data.at("on_ground"));
		entity_pos new_pos;
		new_pos.x = x;
		new_pos.y = feety;
		new_pos.z = z;
		new_pos.yaw = yaw;
		new_pos.pitch = pitch;
		game_event move_and_look_event =
			game_event::player_move_and_look(this->player, new_pos);
		this->server.game.queue_event(move_and_look_event);
	}
	void client::Handle_PLAY_PlayerDigging(const packet &packet)
	{
		// The action the player is taking against the block
		int8_t status = std::get<int8_t>(packet.data.at("status"));
		// Block position
		int32_t x = std::get<int32_t>(packet.data.at("x"));
		// Block position
		uint8_t y = std::get<uint8_t>(packet.data.at("y"));
		// Block position
		int32_t z = std::get<int32_t>(packet.data.at("z"));
		// The face being hit
		int8_t face = std::get<int8_t>(packet.data.at("face"));
	}
	void client::Handle_PLAY_HeldItemChange(const packet &packet)
	{
		// The slot which the player has selected (0-8)
		int16_t slot = std::get<int16_t>(packet.data.at("slot"));
	}
	void client::Handle_PLAY_Animation(const packet &packet)
	{
		// Player ID
		int32_t entity_id = std::get<int32_t>(packet.data.at("entity_id"));
		// Animation ID
		int8_t animation = std::get<int8_t>(packet.data.at("animation"));
	}

	void client::Handle_PLAY_EntityAction(const packet &packet)
	{
		// Player ID
		int32_t entity_id = std::get<int32_t>(packet.data.at("entity_id"));
		// The ID of the action
		int8_t action_id = std::get<int8_t>(packet.data.at("action_id"));
		// Horse jump boost. Ranged from 0 -> 100.
		int32_t jump_boost = std::get<int32_t>(packet.data.at("jump_boost"));
	}

	void client::Handle_PLAY_SteerVehicle(const packet &packet)
	{
		// Positive to the left of the player
		float sideways = std::get<float>(packet.data.at("sideways"));
		// Positive forward
		float forward = std::get<float>(packet.data.at("forward"));
		bool jump = std::get<bool>(packet.data.at("jump"));
		// True when leaving the vehicle
		bool unmount = std::get<bool>(packet.data.at("unmount"));
	}
	void client::Handle_PLAY_CloseWindow(const packet &packet)
	{
		// This is the id of the window that was closed. 0 for inventory.
		int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
	}
	void client::Handle_PLAY_ConfirmTransaction(const packet &packet)
	{
		// The id of the window that the action occurred in.
		int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
		// Every action that is to be accepted has a unique number. This field
		// corresponds to that number.
		int16_t action_number =
			std::get<int16_t>(packet.data.at("action_number"));
		// Whether the action was accepted.
		bool accepted = std::get<bool>(packet.data.at("accepted"));
	}
	void client::Handle_PLAY_EnchantItem(const packet &packet)
	{
		// The ID sent by Open Window
		int8_t window_id = std::get<int8_t>(packet.data.at("window_id"));
		// The position of the enchantment on the enchantment table window,
		// starting with 0 as the topmost one.
		int8_t enchantment = std::get<int8_t>(packet.data.at("enchantment"));
	}
	void client::Handle_PLAY_UpdateSign(const packet &packet)
	{
		// Block X Coordinate
		int32_t x = std::get<int32_t>(packet.data.at("x"));
		// Block Y Coordinate
		int16_t y = std::get<int16_t>(packet.data.at("y"));
		// Block Z Coordinate
		int32_t z = std::get<int32_t>(packet.data.at("z"));
		// First line of text in the sign
		std::string line_1 = std::get<std::string>(packet.data.at("line_1"));
		// Second line of text in the sign
		std::string line_2 = std::get<std::string>(packet.data.at("line_2"));
		// Third line of text in the sign
		std::string line_3 = std::get<std::string>(packet.data.at("line_3"));
		// Fourth line of text in the sign
		std::string line_4 = std::get<std::string>(packet.data.at("line_4"));
	}
	void client::Handle_PLAY_PlayerAbilities(const packet &packet)
	{
		int8_t flags = std::get<int8_t>(packet.data.at("flags"));
		// previous integer value divided by 250
		float flying_speed = std::get<float>(packet.data.at("flying_speed"));
		// previous integer value divided by 250
		float walking_speed = std::get<float>(packet.data.at("walking_speed"));
	}
	void client::Handle_PLAY_TabComplete(const packet &packet)
	{
		std::string text = std::get<std::string>(packet.data.at("text"));
	}
	void client::Handle_PLAY_ClientSettings(const packet &packet)
	{
		// en_GB
		std::string locale = std::get<std::string>(packet.data.at("locale"));
		// Client-side render distance(chunks)
		int8_t view_distance =
			std::get<int8_t>(packet.data.at("view_distance"));
		// Chat settings
		int8_t chat_flags = std::get<int8_t>(packet.data.at("chat_flags"));
		//"Colours" multiplayer setting
		bool chat_colours = std::get<bool>(packet.data.at("chat_colours"));
		// Client-side difficulty from options.txt
		int8_t difficulty = std::get<int8_t>(packet.data.at("difficulty"));
		// Show Cape multiplayer setting
		bool show_cape = std::get<bool>(packet.data.at("show_cape"));
	}
	void client::Handle_PLAY_ClientStatus(const packet &packet)
	{
		int8_t action_id = std::get<int8_t>(packet.data.at("action_id"));
	}
} // namespace wild

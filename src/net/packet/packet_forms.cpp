#include "common.hpp"
#include "net/packet/packet.hpp"

namespace wild
{
	packet_form forms[FORMS_SIZE] = {
		{"handshake",
		 client_state::HANDSHAKING,
		 0x00,
		 4,
		 {{"protocol_version", data_type::VARINT},
		  {"server_address", data_type::STRING},
		  {"server_port", data_type::UNSIGNED_SHORT},
		  {"next_state", data_type::VARINT}}},
		{"keep_alive",
		 client_state::PLAY,
		 0x00,
		 1,
		 {{"keep_alive_id", data_type::INT}}},
		{"chat_message",
		 client_state::PLAY,
		 0x01,
		 1,
		 {{"message", data_type::STRING}}},
		{"use_entity",
		 client_state::PLAY,
		 0x02,
		 2,
		 {{"target", data_type::INT}, {"mouse", data_type::BYTE}}},
		{"player",
		 client_state::PLAY,
		 0x03,
		 1,
		 {{"on_ground", data_type::BOOL}}},
		{"player_position",
		 client_state::PLAY,
		 0x04,
		 5,
		 {{"x", data_type::DOUBLE},
		  {"feety", data_type::DOUBLE},
		  {"heady", data_type::DOUBLE},
		  {"z", data_type::DOUBLE},
		  {"on_ground", data_type::BOOL}}},
		{"player_look",
		 client_state::PLAY,
		 0x05,
		 3,
		 {{"yaw", data_type::FLOAT},
		  {"pitch", data_type::FLOAT},
		  {"on_ground", data_type::BOOL}}},
		{"player_position_and_look",
		 client_state::PLAY,
		 0x06,
		 7,
		 {{"x", data_type::DOUBLE},
		  {"feety", data_type::DOUBLE},
		  {"heady", data_type::DOUBLE},
		  {"z", data_type::DOUBLE},
		  {"yaw", data_type::FLOAT},
		  {"pitch", data_type::FLOAT},
		  {"on_ground", data_type::BOOL}}},
		{"player_digging",
		 client_state::PLAY,
		 0x07,
		 5,
		 {{"status", data_type::BYTE},
		  {"x", data_type::INT},
		  {"y", data_type::UNSIGNED_BYTE},
		  {"z", data_type::INT},
		  {"face", data_type::BYTE}}},
		{"held_item_change",
		 client_state::PLAY,
		 0x09,
		 1,
		 {{"slot", data_type::SHORT}}},
		{"animation",
		 client_state::PLAY,
		 0x0A,
		 2,
		 {{"entity_id", data_type::INT}, {"animation", data_type::BYTE}}},
		{"entity_action",
		 client_state::PLAY,
		 0x0B,
		 3,
		 {{"entity_id", data_type::INT},
		  {"action_id", data_type::BYTE},
		  {"jump_boost", data_type::INT}}},
		{"steer_vehicle",
		 client_state::PLAY,
		 0x0C,
		 4,
		 {{"sideways", data_type::FLOAT},
		  {"forward", data_type::FLOAT},
		  {"jump", data_type::BOOL},
		  {"unmount", data_type::BOOL}}},
		{"close_window",
		 client_state::PLAY,
		 0x0D,
		 1,
		 {{"window_id", data_type::BYTE}}},
		{"confirm_transaction",
		 client_state::PLAY,
		 0x0F,
		 3,
		 {{"window_id", data_type::BYTE},
		  {"action_number", data_type::SHORT},
		  {"accepted", data_type::BOOL}}},
		{"enchant_item",
		 client_state::PLAY,
		 0x11,
		 2,
		 {{"window_id", data_type::BYTE}, {"enchantment", data_type::BYTE}}},
		{"update_sign",
		 client_state::PLAY,
		 0x12,
		 7,
		 {{"x", data_type::INT},
		  {"y", data_type::SHORT},
		  {"z", data_type::INT},
		  {"line_1", data_type::STRING},
		  {"line_2", data_type::STRING},
		  {"line_3", data_type::STRING},
		  {"line_4", data_type::STRING}}},
		{"player_abilities",
		 client_state::PLAY,
		 0x13,
		 3,
		 {{"flags", data_type::BYTE},
		  {"flying_speed", data_type::FLOAT},
		  {"walking_speed", data_type::FLOAT}}},
		{"tab-complete",
		 client_state::PLAY,
		 0x14,
		 1,
		 {{"text", data_type::STRING}}},
		{"client_settings",
		 client_state::PLAY,
		 0x15,
		 6,
		 {{"locale", data_type::STRING},
		  {"view_distance", data_type::BYTE},
		  {"chat_flags", data_type::BYTE},
		  {"chat_colours", data_type::BOOL},
		  {"difficulty", data_type::BYTE},
		  {"show_cape", data_type::BOOL}}},
		{"client_status",
		 client_state::PLAY,
		 0x16,
		 1,
		 {{"action_id", data_type::BYTE}}},
		{"request", client_state::STATUS, 0x00, 0, {}},
		{"ping", client_state::STATUS, 0x01, 1, {{"time", data_type::LONG}}},
		{"login_start",
		 client_state::LOGIN,
		 0x00,
		 1,
		 {{"name", data_type::STRING}}}};
}

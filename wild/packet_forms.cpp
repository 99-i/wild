#include "client.h"
#include "packet.h"

w_packet_form forms[FORMS_SIZE] = {
	{
		"handshake",
		w_client::client_state::HANDSHAKING,
		0x00,
		4,
		{
			{"protocol_version", w_data_type::VARINT},
			{"server_address", w_data_type::STRING},
			{"server_port", w_data_type::UNSIGNED_SHORT},
			{"next_state", w_data_type::VARINT}
		}
	},
	{
		"keep_alive",
		w_client::client_state::PLAY,
		0x00,
		1,
		{
			{"keep_alive_id", w_data_type::INT}
		}
	},
	{
		"chat_message",
		w_client::client_state::PLAY,
		0x01,
		1,
		{
			{"message", w_data_type::STRING}
		}
	},
	{
		"use_entity",
		w_client::client_state::PLAY,
		0x02,
		2,
		{
			{"target", w_data_type::INT},
			{"mouse", w_data_type::BYTE}
		}
	},
	{
		"player",
		w_client::client_state::PLAY,
		0x03,
		1,
		{
			{"on_ground", w_data_type::BOOL}
		}
	},
	{
		"player_position",
		w_client::client_state::PLAY,
		0x04,
		5,
		{
			{"x", w_data_type::DOUBLE},
			{"feety", w_data_type::DOUBLE},
			{"heady", w_data_type::DOUBLE},
			{"z", w_data_type::DOUBLE},
			{"on_ground", w_data_type::BOOL}
		}
	},
	{
		"player_look",
		w_client::client_state::PLAY,
		0x05,
		3,
		{
			{"yaw", w_data_type::FLOAT},
			{"pitch", w_data_type::FLOAT},
			{"on_ground", w_data_type::BOOL}
		}
	},
	{
		"player_position_and_look",
		w_client::client_state::PLAY,
		0x06,
		7,
		{
			{"x", w_data_type::DOUBLE},
			{"feety", w_data_type::DOUBLE},
			{"heady", w_data_type::DOUBLE},
			{"z", w_data_type::DOUBLE},
			{"yaw", w_data_type::FLOAT},
			{"pitch", w_data_type::FLOAT},
			{"on_ground", w_data_type::BOOL}
		}
	},
	{
		"player_digging",
		w_client::client_state::PLAY,
		0x07,
		5,
		{
			{"status", w_data_type::BYTE},
			{"x", w_data_type::INT},
			{"y", w_data_type::UNSIGNED_BYTE},
			{"z", w_data_type::INT},
			{"face", w_data_type::BYTE}
		}
	},
	{
		"held_item_change",
		w_client::client_state::PLAY,
		0x09,
		1,
		{
			{"slot", w_data_type::SHORT}
		}
	},
	{
		"animation",
		w_client::client_state::PLAY,
		0x0A,
		2,
		{
			{"entity_id", w_data_type::INT},
			{"animation", w_data_type::BYTE}
		}
	},
	{
		"entity_action",
		w_client::client_state::PLAY,
		0x0B,
		3,
		{
			{"entity_id", w_data_type::INT},
			{"action_id", w_data_type::BYTE},
			{"jump_boost", w_data_type::INT}
		}
	},
	{
		"steer_vehicle",
		w_client::client_state::PLAY,
		0x0C,
		4,
		{
			{"sideways", w_data_type::FLOAT},
			{"forward", w_data_type::FLOAT},
			{"jump", w_data_type::BOOL},
			{"unmount", w_data_type::BOOL}
		}
	},
	{
		"close_window",
		w_client::client_state::PLAY,
		0x0D,
		1,
		{
			{"window_id", w_data_type::BYTE}
		}
	},
	{
		"confirm_transaction",
		w_client::client_state::PLAY,
		0x0F,
		3,
		{
			{"window_id", w_data_type::BYTE},
			{"action_number", w_data_type::SHORT},
			{"accepted", w_data_type::BOOL}
		}
	},
	{
		"enchant_item",
		w_client::client_state::PLAY,
		0x11,
		2,
		{
			{"window_id", w_data_type::BYTE},
			{"enchantment", w_data_type::BYTE}
		}
	},
	{
		"update_sign",
		w_client::client_state::PLAY,
		0x12,
		7,
		{
			{"x", w_data_type::INT},
			{"y", w_data_type::SHORT},
			{"z", w_data_type::INT},
			{"line_1", w_data_type::STRING},
			{"line_2", w_data_type::STRING},
			{"line_3", w_data_type::STRING},
			{"line_4", w_data_type::STRING}
		}
	},
	{
		"player_abilities",
		w_client::client_state::PLAY,
		0x13,
		3,
		{
			{"flags", w_data_type::BYTE},
			{"flying_speed", w_data_type::FLOAT},
			{"walking_speed", w_data_type::FLOAT}
		}
	},
	{
		"tab-complete",
		w_client::client_state::PLAY,
		0x14,
		1,
		{
			{"text", w_data_type::STRING}
		}
	},
	{
		"client_settings",
		w_client::client_state::PLAY,
		0x15,
		6,
		{
			{"locale", w_data_type::STRING},
			{"view_distance", w_data_type::BYTE},
			{"chat_flags", w_data_type::BYTE},
			{"chat_colours", w_data_type::BOOL},
			{"difficulty", w_data_type::BYTE},
			{"show_cape", w_data_type::BOOL}
		}
	},
	{
		"client_status",
		w_client::client_state::PLAY,
		0x16,
		1,
		{
			{"action_id", w_data_type::BYTE}
		}
	},
	{
		"request",
		w_client::client_state::STATUS,
		0x00,
		0,
		{
		}
	},
	{
		"ping",
		w_client::client_state::STATUS,
		0x01,
		1,
		{
			{"time", w_data_type::LONG}
		}
	},
	{
		"login_start",
		w_client::client_state::LOGIN,
		0x00,
		1,
		{
			{"name", w_data_type::STRING}
		}
	}
};
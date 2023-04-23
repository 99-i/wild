#include "common.h"
#include "client.h"
#include "packet.h"

wild::packet_form wild::forms[wild::FORMS_SIZE] = {
	{
		"handshake",
		client_state::HANDSHAKING,
		0x00,
		4,
		{
			{"protocol_version", wild::data_type::VARINT},
			{"server_address", wild::data_type::STRING},
			{"server_port", wild::data_type::UNSIGNED_SHORT},
			{"next_state", wild::data_type::VARINT}
		}
	},
	{
		"keep_alive",
		client_state::PLAY,
		0x00,
		1,
		{
			{"keep_alive_id", wild::data_type::INT}
		}
	},
	{
		"chat_message",
		client_state::PLAY,
		0x01,
		1,
		{
			{"message", wild::data_type::STRING}
		}
	},
	{
		"use_entity",
		client_state::PLAY,
		0x02,
		2,
		{
			{"target", wild::data_type::INT},
			{"mouse", wild::data_type::BYTE}
		}
	},
	{
		"player",
		client_state::PLAY,
		0x03,
		1,
		{
			{"on_ground", wild::data_type::BOOL}
		}
	},
	{
		"player_position",
		client_state::PLAY,
		0x04,
		5,
		{
			{"x", wild::data_type::DOUBLE},
			{"feety", wild::data_type::DOUBLE},
			{"heady", wild::data_type::DOUBLE},
			{"z", wild::data_type::DOUBLE},
			{"on_ground", wild::data_type::BOOL}
		}
	},
	{
		"player_look",
		client_state::PLAY,
		0x05,
		3,
		{
			{"yaw", wild::data_type::FLOAT},
			{"pitch", wild::data_type::FLOAT},
			{"on_ground", wild::data_type::BOOL}
		}
	},
	{
		"player_position_and_look",
		client_state::PLAY,
		0x06,
		7,
		{
			{"x", wild::data_type::DOUBLE},
			{"feety", wild::data_type::DOUBLE},
			{"heady", wild::data_type::DOUBLE},
			{"z", wild::data_type::DOUBLE},
			{"yaw", wild::data_type::FLOAT},
			{"pitch", wild::data_type::FLOAT},
			{"on_ground", wild::data_type::BOOL}
		}
	},
	{
		"player_digging",
		client_state::PLAY,
		0x07,
		5,
		{
			{"status", wild::data_type::BYTE},
			{"x", wild::data_type::INT},
			{"y", wild::data_type::UNSIGNED_BYTE},
			{"z", wild::data_type::INT},
			{"face", wild::data_type::BYTE}
		}
	},
	{
		"held_item_change",
		client_state::PLAY,
		0x09,
		1,
		{
			{"slot", wild::data_type::SHORT}
		}
	},
	{
		"animation",
		client_state::PLAY,
		0x0A,
		2,
		{
			{"entity_id", wild::data_type::INT},
			{"animation", wild::data_type::BYTE}
		}
	},
	{
		"entity_action",
		client_state::PLAY,
		0x0B,
		3,
		{
			{"entity_id", wild::data_type::INT},
			{"action_id", wild::data_type::BYTE},
			{"jump_boost", wild::data_type::INT}
		}
	},
	{
		"steer_vehicle",
		client_state::PLAY,
		0x0C,
		4,
		{
			{"sideways", wild::data_type::FLOAT},
			{"forward", wild::data_type::FLOAT},
			{"jump", wild::data_type::BOOL},
			{"unmount", wild::data_type::BOOL}
		}
	},
	{
		"close_window",
		client_state::PLAY,
		0x0D,
		1,
		{
			{"window_id", wild::data_type::BYTE}
		}
	},
	{
		"confirm_transaction",
		client_state::PLAY,
		0x0F,
		3,
		{
			{"window_id", wild::data_type::BYTE},
			{"action_number", wild::data_type::SHORT},
			{"accepted", wild::data_type::BOOL}
		}
	},
	{
		"enchant_item",
		client_state::PLAY,
		0x11,
		2,
		{
			{"window_id", wild::data_type::BYTE},
			{"enchantment", wild::data_type::BYTE}
		}
	},
	{
		"update_sign",
		client_state::PLAY,
		0x12,
		7,
		{
			{"x", wild::data_type::INT},
			{"y", wild::data_type::SHORT},
			{"z", wild::data_type::INT},
			{"line_1", wild::data_type::STRING},
			{"line_2", wild::data_type::STRING},
			{"line_3", wild::data_type::STRING},
			{"line_4", wild::data_type::STRING}
		}
	},
	{
		"player_abilities",
		client_state::PLAY,
		0x13,
		3,
		{
			{"flags", wild::data_type::BYTE},
			{"flying_speed", wild::data_type::FLOAT},
			{"walking_speed", wild::data_type::FLOAT}
		}
	},
	{
		"tab-complete",
		client_state::PLAY,
		0x14,
		1,
		{
			{"text", wild::data_type::STRING}
		}
	},
	{
		"client_settings",
		client_state::PLAY,
		0x15,
		6,
		{
			{"locale", wild::data_type::STRING},
			{"view_distance", wild::data_type::BYTE},
			{"chat_flags", wild::data_type::BYTE},
			{"chat_colours", wild::data_type::BOOL},
			{"difficulty", wild::data_type::BYTE},
			{"show_cape", wild::data_type::BOOL}
		}
	},
	{
		"client_status",
		client_state::PLAY,
		0x16,
		1,
		{
			{"action_id", wild::data_type::BYTE}
		}
	},
	{
		"request",
		client_state::STATUS,
		0x00,
		0,
		{
		}
	},
	{
		"ping",
		client_state::STATUS,
		0x01,
		1,
		{
			{"time", wild::data_type::LONG}
		}
	},
	{
		"login_start",
		client_state::LOGIN,
		0x00,
		1,
		{
			{"name", wild::data_type::STRING}
		}
	}
};
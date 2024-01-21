#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace wild
{
	namespace metadata
	{
		struct entity_metadata_writer
		{
		private:
			enum entity_metadata_type : uint8_t
			{
				BYTE = 0,
				SHORT = 1,
				INT = 2,
				FLOAT = 3,
				STRING = 4,
				//todo
				//SLOT
			};

		public:
			typedef std::variant<int8_t, int16_t, int32_t, float, std::string> data;
			std::map<uint8_t, std::tuple<entity_metadata_type, data>> _data;

			void set_i8(uint8_t index, int8_t i8);
			void set_i16(uint8_t index, int16_t i16);
			void set_i32(uint8_t index, int32_t i32);
			void set_f32(uint8_t index, float f32);
			void set_string(uint8_t index, std::string string);

			//todo
			//void set_slot(uint32_t index, slot slot);

			//returns the bytes of this (to send to client)
			std::vector<uint8_t> compile() const;
		};

		struct metadata_base
		{
		protected:
			metadata::entity_metadata_writer writer;
		public:
			std::vector<uint8_t> get_data() const
			{
				return this->writer.compile();
			}
		};

		struct entity_metadata : public metadata_base
		{
			enum status_flags
			{
				ON_FIRE = 0x01,
				CROUCHED = 0x02,
				SPRINTING = 0x08,
				EATING_DRINKING_BLOCKING = 0x10,
				INVISIBLE = 0x20
			};

			void set_status(int8_t status);
			int8_t get_status();

			void set_air(int16_t air);
			int16_t get_air();
		};
		struct living_entity_metadata : public entity_metadata
		{
			void set_health(float health);
			float get_health();

			void set_potion_effect_color(int32_t potion_effect_color);
			int32_t get_potion_effect_color();

			void set_is_potion_effect_ambient(int8_t is_potion_effect_ambient);
			int8_t get_is_potion_effect_ambient();

			void set_number_of_arrows_in_entity(int8_t number_of_arrows_in_entity);
			int8_t get_number_of_arrows_in_entity();

			void set_name_tag(std::string name_tag);
			std::string get_name_tag();

			void set_always_show_name_tag(int8_t always_show_name_tag);
			int8_t get_always_show_name_tag();
		};
		struct human_metadata : public living_entity_metadata
		{
			enum
			{
				HIDE_CAPE = 0x02
			};
			void set_hide_cape(int8_t hide_cape);
			int8_t get_hide_cape();

			void set_absorption_hearts(float absorption_hearts);
			float get_absorption_hearts();

			void set_score(int32_t score);
			int32_t get_score();
		};
		//todo
	}
}
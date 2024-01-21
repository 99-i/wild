#include "game/entity/entity_metadata.hpp"
#include "net/packet/clientbound_packet.hpp"

namespace wild
{
	void metadata::entity_metadata_writer::set_i8(uint8_t index, int8_t i8)
	{
		this->_data.emplace(std::make_pair(
			index,
			std::make_tuple(entity_metadata_type::BYTE,
							metadata::entity_metadata_writer::data(i8))));
	}
	void metadata::entity_metadata_writer::set_i16(uint8_t index, int16_t i16)
	{
		this->_data.emplace(std::make_pair(
			index,
			std::make_tuple(entity_metadata_type::SHORT,
							metadata::entity_metadata_writer::data(i16))));
	}
	void metadata::entity_metadata_writer::set_i32(uint8_t index, int32_t i32)
	{
		this->_data.emplace(std::make_pair(
			index,
			std::make_tuple(entity_metadata_type::INT,
							metadata::entity_metadata_writer::data(i32))));
	}
	void metadata::entity_metadata_writer::set_f32(uint8_t index, float f32)
	{
		this->_data.emplace(std::make_pair(
			index,
			std::make_tuple(entity_metadata_type::FLOAT,
							metadata::entity_metadata_writer::data(f32))));
	}
	void metadata::entity_metadata_writer::set_string(uint8_t index,
													  std::string string)
	{
		this->_data.emplace(std::make_pair(
			index,
			std::make_tuple(entity_metadata_type::STRING,
							metadata::entity_metadata_writer::data(string))));
	}

	std::vector<uint8_t> metadata::entity_metadata_writer::compile() const
	{
		std::vector<uint8_t> compiled_data;
		for (auto it = this->_data.begin(); it != this->_data.end(); it++)
			{
				uint8_t index = it->first;
				uint8_t type_index = std::get<0>(it->second);
				data d = std::get<1>(it->second);

				// the first byte.
				uint8_t byte = ((type_index << 5) | (index & 0x1F)) & 0xFF;
				std::vector<uint8_t> tmp;
				compiled_data.push_back(byte);
				switch (type_index)
					{
					case entity_metadata_writer::entity_metadata_type::BYTE:
						tmp = write_fn::write_i8(std::get<int8_t>(d));
						break;
					case entity_metadata_writer::entity_metadata_type::SHORT:
						tmp = write_fn::write_i16(std::get<int16_t>(d));
						break;
					case entity_metadata_writer::entity_metadata_type::INT:
						tmp = write_fn::write_i32(std::get<int32_t>(d));
						break;
					case entity_metadata_writer::entity_metadata_type::FLOAT:
						tmp = write_fn::write_float(std::get<float>(d));
						break;
					case entity_metadata_writer::entity_metadata_type::STRING:
						tmp = write_fn::write_string(std::get<std::string>(d));
						break;
					}
				compiled_data.insert(compiled_data.end(), tmp.begin(),
									 tmp.end());
			}
		compiled_data.push_back(0x7F);
		return compiled_data;
	}

	void metadata::entity_metadata::set_status(int8_t status)
	{
		this->writer.set_i8(0, status);
	}
	int8_t metadata::entity_metadata::get_status()
	{
		return std::get<int8_t>(std::get<1>(this->writer._data.at(0)));
	}
	void metadata::entity_metadata::set_air(int16_t air)
	{
		this->writer.set_i16(1, air);
	}
	int16_t metadata::entity_metadata::get_air()
	{
		return std::get<int16_t>(std::get<1>(this->writer._data.at(1)));
	}
	void metadata::living_entity_metadata::set_health(float health)
	{
		this->writer.set_f32(6, health);
	}
	float metadata::living_entity_metadata::get_health()
	{
		return std::get<float>(std::get<1>(this->writer._data.at(6)));
	}
	void metadata::living_entity_metadata::set_potion_effect_color(
		int32_t potion_effect_color)
	{
		this->writer.set_i32(7, potion_effect_color);
	}
	int32_t metadata::living_entity_metadata::get_potion_effect_color()
	{
		return std::get<int32_t>(std::get<1>(this->writer._data.at(7)));
	}
	void metadata::living_entity_metadata::set_is_potion_effect_ambient(
		int8_t is_potion_effect_ambient)
	{
		this->writer.set_i8(8, is_potion_effect_ambient);
	}
	int8_t metadata::living_entity_metadata::get_is_potion_effect_ambient()
	{
		return std::get<int8_t>(std::get<1>(this->writer._data.at(8)));
	}
	void metadata::living_entity_metadata::set_number_of_arrows_in_entity(
		int8_t number_of_arrows_in_entity)
	{
		this->writer.set_i8(9, number_of_arrows_in_entity);
	}
	int8_t metadata::living_entity_metadata::get_number_of_arrows_in_entity()
	{
		return std::get<int8_t>(std::get<1>(this->writer._data.at(9)));
	}
	void metadata::living_entity_metadata::set_name_tag(std::string name_tag)
	{
		this->writer.set_string(10, name_tag);
	}
	std::string metadata::living_entity_metadata::get_name_tag()
	{
		return std::get<std::string>(std::get<1>(this->writer._data.at(10)));
	}
	void metadata::living_entity_metadata::set_always_show_name_tag(
		int8_t always_show_name_tag)
	{
		this->writer.set_i8(11, always_show_name_tag);
	}
	int8_t metadata::living_entity_metadata::get_always_show_name_tag()
	{
		return std::get<int8_t>(std::get<1>(this->writer._data.at(11)));
	}
	void metadata::human_metadata::set_hide_cape(int8_t hide_cape)
	{
		this->writer.set_i8(16, hide_cape);
	}
	int8_t metadata::human_metadata::get_hide_cape()
	{
		return std::get<int8_t>(std::get<1>(this->writer._data.at(16)));
	}
	void
	metadata::human_metadata::set_absorption_hearts(float absorption_hearts)
	{
		this->writer.set_f32(17, absorption_hearts);
	}
	float metadata::human_metadata::get_absorption_hearts()
	{
		return std::get<float>(std::get<1>(this->writer._data.at(17)));
	}
	void metadata::human_metadata::set_score(int32_t score)
	{
		this->writer.set_i32(18, score);
	}
	int32_t metadata::human_metadata::get_score()
	{
		return std::get<int32_t>(std::get<1>(this->writer._data.at(18)));
	}
} // namespace wild

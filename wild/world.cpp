#include "world.h"

void w_world::load_chunk(int32_t x, uint8_t y, int32_t z)
{
	//check if chunk is already loaded.
	for (auto it = this->loaded_chunks.begin(); it != this->loaded_chunks.end(); it++)
	{
		if ((*it)->x == x && (*it)->y == y && (*it)->z == z)
		{
			//chunk is already loaded.
			return;
		}
	}
	w_chunk *chunk = new w_chunk(x, y, z);
	this->loaded_chunks.push_back(chunk);
}
void w_world::unload_chunk(int32_t x, uint8_t y, int32_t z)
{
	for (auto it = this->loaded_chunks.begin(); it != this->loaded_chunks.end(); it++)
	{
		if ((*it)->x == x && (*it)->y == y && (*it)->z == z)
		{
			//delete the chunk
			delete *it;
			this->loaded_chunks.erase(it);
			return;
		}
	}
}

std::optional<block_t> w_world::get_block_at(vec3i pos)
{
	//get logical chunk coords.
	int32_t chunk_x = pos.x / 16;
	uint8_t chunk_y = pos.y / 16;
	int32_t chunk_z = pos.z / 16;

	auto loaded_chunk = std::find_if(this->loaded_chunks.begin(), this->loaded_chunks.end(), [&](w_chunk *chunk)
		{
			return (chunk->x == chunk_x && chunk->y == chunk_y && chunk->z == chunk_z);
		});

	if (loaded_chunk == this->loaded_chunks.end())
		return std::nullopt;

	int32_t x_offset = pos.x - (chunk_x * 16);
	int32_t y_offset = pos.y - (chunk_y * 16);
	int32_t z_offset = pos.z - (chunk_z * 16);

	return std::make_tuple((*loaded_chunk)->block_ids[x_offset, y_offset, z_offset], (*loaded_chunk)->block_data[x_offset, y_offset, z_offset]);
}
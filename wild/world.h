#pragma once
#include <optional>
#include <vector>
#include "common.h"

namespace wild
{
	typedef int32_t block_id_t;
	typedef uint8_t block_data_t;
	typedef std::tuple<block_id_t, block_data_t> block_t;
	class chunk;
	//represents an in-memory loaded world.
	class world
	{
		std::vector<chunk *> loaded_chunks;
	public:
		void load_chunk(int32_t x, uint8_t y, int32_t z);
		void unload_chunk(int32_t x, uint8_t y, int32_t z);
		//returns nullopt if chunk not loaded or pos is out of bounds.
		std::optional<block_t> get_block_at(vec3i pos);
	};

	struct chunk
	{
		world *world;

		//block ids.
		//blocks[x][y][z]
		block_id_t *block_ids;
		//block data bytes.
		//block_data[x][y][z]
		block_data_t *block_data;

	public:
		//the logical x coord of the whole chunk.
		int32_t x;
		//the logical y coord of the whole chunk, from 0 to 15.
		uint8_t y;
		//the logical z coord of the whole chunk.
		int32_t z;

		//return the raw chunk data.
		uint8_t *get_chunk_data()
		{
			//TODO
			return nullptr;
		}

		bool is_empty()
		{
			for (size_t i = 0; i < 16 * 16 * 16; i++)
			{
				if (this->block_data[i] != 0 || this->block_ids[i] != 0)
					return false;
			}
			return true;
		}

		chunk(int32_t x, uint8_t y, int32_t z) : x(x), y(y), z(z)
		{
			this->block_ids = new block_id_t[16 * 16 * 16];
			this->block_data = new block_data_t[16 * 16 * 16];
		}
		~chunk()
		{
			delete[] this->block_ids;
			delete[] this->block_data;
		}
	};
}
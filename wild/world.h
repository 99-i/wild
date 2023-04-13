#pragma once
#include <vector>
#include <array>
#include <optional>
#include <cstdint>

typedef int32_t block_id_t;
typedef uint8_t block_data_t;
typedef std::tuple<block_id_t, block_data_t> block_t;
//represents an in-memory loaded world.

struct vec3i
{
	int x;
	int y;
	int z;
	vec3i(int x, int y, int z) : x(x), y(y), z(z)
	{
	}
	bool operator==(const vec3i &other)
	{
		return (other.x == this->x && other.y == this->y && other.z == this->z);
	}
};
struct vec3f
{
	float x;
	float y;
	float z;
	vec3f(float x, float y, float z) : x(x), y(y), z(z)
	{
	}
	bool operator==(const vec3f &other)
	{
		return (other.x == this->x && other.y == this->y && other.z == this->z);
	}
};

struct w_chunk;
struct w_world
{
	std::vector<w_chunk *> loaded_chunks;

	void load_chunk(int32_t x, uint8_t y, int32_t z);
	void unload_chunk(int32_t x, uint8_t y, int32_t z);

	std::optional<block_t> get_block_at(vec3i pos);
};

struct w_chunk
{
	w_world *world;
	//the logical x coord of the whole chunk.
	int32_t x;
	//the logical y coord of the whole chunk, from 0 to 15.
	uint8_t y;
	//the logical z coord of the whole chunk.
	int32_t z;

	//block ids.
	//blocks[x][y][z]
	block_id_t *block_ids;
	//block data bytes.
	//block_data[x][y][z]
	block_data_t *block_data;

	w_chunk(int32_t x, uint8_t y, int32_t z) : x(x), y(y), z(z)
	{
		this->block_ids = new block_id_t[16 * 16 * 16];
		this->block_data = new block_data_t[16 * 16 * 16];
	}
	~w_chunk()
	{
		delete[] this->block_ids;
		delete[] this->block_data;
	}
};
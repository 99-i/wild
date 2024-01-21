#pragma once
#include "../entity/entity.hpp"
#include "../entity/player.hpp"
#include <map>
#include <mutex>
#include <vector>

namespace wild
{

struct chunk_data
{
};

struct chunk
{
	// bits 0-30: chunk x
	// bits 31-60: chunk z
	// bits 61-64: chunk y
	typedef uint64_t chunk_index;
	// todo
	enum class chunk_state
	{
		UNLOADED,
		LOADED
	};

	chunk_state state;
};

class world
{
	std::vector<wild::entity *> entities;
	std::map<chunk::chunk_index, chunk *> chunks;
	// tick their positions, etc.
	void tick_entities();

  public:
	world() {}
	~world() {}
	// mutex for this->entities
	std::mutex entities_mutex;
	void tick() { this->tick_entities(); }

	std::vector<wild::player *> get_players();

	void spawn_entity(wild::entity *entity);
};
} // namespace wild

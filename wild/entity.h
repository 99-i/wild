#pragma once
#include <cstdint>
#include "common.h"
#include "random.h"

namespace wild
{
	struct entity_pos : public vec3f
	{
		float yaw;
		float pitch;
	};
	//any entity that has an id and position (so all entities). e.g., player, pig, zombie, arrow, etc
	class entity
	{
	protected:
		entity() = default;
		//every entity that's created gets a sequentially generated entity id
		static counter<int32_t> id_counter;
	public:
		int32_t id = id_counter.next();
		//the entity's position on the server
		wild::entity_pos pos;
	};
}

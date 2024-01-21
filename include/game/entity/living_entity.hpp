#pragma once
#include "entity.hpp"

namespace wild
{
	class living_entity : public entity
	{
	public:
		living_entity(entity_type type) : entity(type) {}
		float health;
	};
}

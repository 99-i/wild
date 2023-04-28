#pragma once
#include <cstdint>
#include "common.h"
#include "random.h"

namespace wild
{
	class entity
	{
	public:
		static counter<int32_t> id_counter;
		int32_t id = id_counter.next();
		wild::vec3f pos;
	};
}

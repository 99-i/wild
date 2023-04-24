#pragma once
#include <cstdint>
#include "common.h"

namespace wild
{
	class entity
	{
	public:
		//hi then lo
		uint64_t uuid[2];
		wild::vec3f pos;
	};
}

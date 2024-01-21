#pragma once
#include <random>

namespace wild
{
	class Random
	{
		static int64_t random_integer()
		{
			std::uniform_int_distribution<int64_t> uni(INT64_MIN, INT64_MAX);
			static std::random_device rd;
			static auto rng = std::mt19937(rd());
			return uni(rng);
		}
		static int64_t random_integer(int64_t min)
		{
			std::uniform_int_distribution<int64_t> uni(min, INT64_MAX);
			static std::random_device rd;
			static auto rng = std::mt19937(rd());
			return uni(rng);
		}
		static int64_t random_integer(int64_t min, int64_t max)
		{
			std::uniform_int_distribution<int64_t> uni(min, max);
			static std::random_device rd;
			static auto rng = std::mt19937(rd());
			return uni(rng);
		}
	public:
		static uint32_t random_u32()
		{
			return random_integer(0, UINT32_MAX);
		}
		static uint32_t random_u32(uint32_t min, uint32_t max)
		{
			return random_integer(min, max);
		}
		static int32_t random_i32()
		{
			return random_integer(INT32_MIN, INT32_MAX);
		}
		static int32_t random_i32(int32_t min, int32_t max)
		{
			return random_integer(min, max);
		}
	};
}

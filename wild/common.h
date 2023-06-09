#pragma once
#include <functional>
#include <tuple>
#include "util.h"
namespace wild
{
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
		vec3f() : x(0), y(0), z(0)
		{
		}
		vec3f(float x, float y, float z) : x(x), y(y), z(z)
		{
		}
		float distance(const vec3f &other)
		{
			return sqrtf(powf(other.x - this->x, 2) + powf(other.y - this->y, 2) + powf(other.z - this->z, 2));
		}
		bool operator==(const vec3f &other)
		{
			return (other.x == this->x && other.y == this->y && other.z == this->z);
		}
	};

	//helper class for converting to fixed point.
	template <class BaseType, size_t FracDigits>
	class fixed_point
	{
		const static BaseType factor = 1 << FracDigits;

		BaseType data;

	public:
		fixed_point(double d)
		{
			*this = d; // calls operator=
		}

		fixed_point &operator=(double d)
		{
			data = static_cast<BaseType>(d * factor);
			return *this;
		}

		BaseType raw_data() const
		{
			return data;
		}

		// Other operators can be defined here
	};

	enum class client_state
	{
		HANDSHAKING,
		STATUS,
		LOGIN,
		PLAY
	};

	namespace runnable
	{
		enum class run_type
		{
			//run once after delay.
			ONCE,
			//run on a timer, every interval ticks after delay.
			TIMER
		};
		//uint32_t: runnable ID
		//returns: true if runnable should continue (timer), or false if runnable should cancel
		//esentially the return value doesn't matter for ONCE runnables
		typedef std::function<bool(uint32_t)> c_function_t;
		//run type, delay (in ticks), interval (in ticks) (ignored if run_type == ONCE)
		typedef std::tuple<run_type /*run type*/, uint32_t /*delay*/, uint32_t /*interval*/> run_settings_t;
	}
}

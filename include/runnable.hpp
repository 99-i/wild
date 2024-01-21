#pragma once
#include <cstdint>
#include <functional>
#include <tuple>
namespace wild
{
namespace runnable
{
enum class run_type
{
	// run once after delay.
	ONCE,
	// run on a timer, every interval ticks after delay.
	TIMER
};
// uint32_t: runnable ID
// returns: true if runnable should continue (timer), or false if runnable
// should cancel esentially the return value doesn't matter for ONCE runnables
typedef std::function<bool(uint32_t)> c_function_t;
// run type, delay (in ticks), interval (in ticks) (ignored if run_type == ONCE)
typedef std::tuple<run_type /*run type*/, uint32_t /*delay*/,
				   uint32_t /*interval*/>
	run_settings_t;

} // namespace runnable
} // namespace wild

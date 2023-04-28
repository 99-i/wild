#pragma once

namespace wild
{
	template<typename T>
	class counter
	{
		T current = 0;
	public:
		T next()
		{
			return this->current++;
		}
	};
}
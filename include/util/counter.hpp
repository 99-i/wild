#pragma once

namespace wild
{
	//a helper class that just counts up.
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
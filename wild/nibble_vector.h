#pragma once
#include <cassert>
#include <vector>

namespace wild
{
	class nibble_vector
	{
		uint8_t pos = 1; //0 if inserting into the lo bits, 1 if need to create new byte and insert into hi bits
	public:
		void push(uint8_t nibble)
		{
			switch (pos)
			{
				case 0:
				{
					uint8_t existing = this->data[this->data.size() - 1];
					this->data[this->data.size() - 1] = existing | nibble;
				}
				pos = 1;
				break;
				case 1:
					this->data.push_back(nibble << 4);
					pos = 0;
					break;
				default:
					assert(false && "Unreachable.");
					break;
			}
		}
		std::vector<uint8_t> data;
	};
}
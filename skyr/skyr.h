#pragma once
#include <cstdint>

namespace skyr
{
	enum
	{
		OP_RETURN
	};
	class chunk
	{
		uint8_t *code = nullptr;
		size_t count = 0;
		size_t capacity = 0;

	public:
		void write_byte(uint8_t byte);
		~chunk();
#ifdef _DEBUG
	public:
		void write_to_stdout(const char *name);
	private:
		size_t dissassemble_instruction(size_t offset);
	public:
#endif // _DEBUG
	};
}

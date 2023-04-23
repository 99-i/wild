#include <cstdlib>
#include "skyr.h"

int main()
{
	skyr::chunk *chunk = new skyr::chunk();
	for (int i = 0; i < 20; i++)
		chunk->write_byte(skyr::OP_RETURN);
	chunk->write_to_stdout("Test chunk");

	system("pause");
}
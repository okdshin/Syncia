#ifdef HASHID_UNIT_TEST
#include "HashId.h"
#include <iostream>

using namespace syncia::database;

int main(int argc, char* argv[])
{
	const auto hash_id = CalcHashId(CreateByteArrayFromString("helllo"));
	const auto hash_id2 = CalcHashId(CreateByteArrayFromString("helllq"));
	std::cout << hash_id.ToString() << std::endl;
	std::cout << hash_id2.ToString() << std::endl;

    return 0;
}

#endif

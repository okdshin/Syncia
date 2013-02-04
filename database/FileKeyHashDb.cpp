#ifdef FILEKEYHASHDB_UNIT_TEST
#include "FileKeyHashDb.h"
#include <iostream>

using namespace syncia;
using namespace syncia::database;

int main(int argc, char* argv[])
{
	auto database = FileKeyHashDb();
	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	std::cout << database << std::endl;

	return 0;
}

#endif
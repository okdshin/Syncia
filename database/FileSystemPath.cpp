#ifdef FILESYSTEMPATH_UNIT_TEST
#include "FileSystemPath.h"
#include <iostream>

using namespace syncia;
using namespace syncia::database;

int main(int argc, char* argv[])
{
	CreateNecessaryDirectory(FileSystemPath("./test/test/test/test.txt"));
	CreateNecessaryDirectory(FileSystemPath("./test/test/test/test/test.txt"));

    return 0;
}

#endif

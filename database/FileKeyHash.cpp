#ifdef FILEKEYHASH_UNIT_TEST
#include "FileKeyHash.h"
#include <iostream>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace syncia;
using namespace syncia::database;

int main(int argc, char* argv[])
{
	auto src_key_hash = CreateTestFileKeyHash();
	std::cout << "src:" << src_key_hash << std::endl;
	std::cout << src_key_hash.GetLastCheckedTime() << std::endl; 

	std::cout << "IsFileExist:" << IsFileExist(src_key_hash);

	std::stringstream ss;
	boost::archive::text_oarchive oa(ss);
	oa << (const FileKeyHash&)src_key_hash;

	std::cout << "text serialized:" << ss.str() << std::endl;

	auto dst_key_hash = FileKeyHash();
	boost::archive::text_iarchive ia(ss);
	ia >> dst_key_hash;
	std::cout << "dst:" << dst_key_hash << std::endl;
	std::cout << dst_key_hash.GetLastCheckedTime() << std::endl; 
    return 0;
}

#endif

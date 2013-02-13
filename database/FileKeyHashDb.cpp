#ifdef FILEKEYHASHDB_UNIT_TEST
#include "FileKeyHashDb.h"
#include <iostream>
#include <boost/thread.hpp>

using namespace syncia;
using namespace syncia::database;

int main(int argc, char* argv[])
{
	boost::asio::io_service io_service;
	boost::asio::io_service::work work(io_service);
	FileKeyHashDb database(io_service);
	boost::thread thread(boost::bind(&boost::asio::io_service::run, &io_service));

	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	database.Add(CreateTestFileKeyHash());
	std::cout << database << std::endl;
	database.QuoteSearch(Keyword("test"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << file_key_hash_list << std::endl;
	});
	database.QuoteSearch(Keyword("test hello"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << file_key_hash_list << std::endl;
	});
	database.QuoteSearch(Keyword("t est"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << file_key_hash_list << std::endl;
	});

	thread.join();
	return 0;
}

#endif

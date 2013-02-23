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
	/*
	auto database = FileKeyHashDb::Create(io_service, 
		FileKeyHashAdder(), FileKeyHashRemover(), FileKeyHashListQuoter());
	*/
	boost::thread thread(boost::bind(&boost::asio::io_service::run, &io_service));
	auto database = CreateBasicFileKeyHashDb(io_service);
	database->Add(CreateTestFileKeyHash());
	database->Add(CreateTestFileKeyHash());
	database->Add(CreateTestFileKeyHash());
	database->Add(CreateTestFileKeyHash());
	std::cout << database << std::endl;
	database->QuoteSearch(Keyword("test"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << "keyword:test" << std::endl;
		std::cout << file_key_hash_list << std::endl;
	});

	database->QuoteSearch(Keyword("test hello"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << "keyword:test hello" << std::endl;
		std::cout << file_key_hash_list << std::endl;
	});
	
	database->QuoteSearch(Keyword("t est"), 
			[](const FileKeyHashList& file_key_hash_list){
		std::cout << "keyword:t est" << std::endl;
		std::cout << file_key_hash_list << std::endl;
	});

	thread.join();
	return 0;
}

#endif

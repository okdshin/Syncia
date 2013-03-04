#ifdef LINKDB_UNIT_TEST
#include "LinkDb.h"
#include <iostream>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace syncia;
using namespace syncia::config;

int main(int argc, char* argv[])
{
	boost::asio::io_service io_service;
	boost::asio::io_service::work work(io_service);
	auto link_db = CreateBasicLinkDb(io_service);
	link_db->Add(NodeId("hello nodeid"));
	boost::thread thread(boost::bind(&boost::asio::io_service::run, &io_service));
	std::cout << link_db << std::endl;
	thread.join();

    return 0;
}

#endif

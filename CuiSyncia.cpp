#ifdef CUISYNCIA_UNIT_TEST
#include "CuiSyncia.h"
#include <iostream>

using namespace syncia;
using namespace neuria::network;

int main(int argc, char* argv[])
{
	HostName host_name("172.16.11.148");	
	PortNumber port_number(54321);
	
	if(argc > 2){
		host_name = HostName(std::string(argv[1]));
		port_number = (boost::lexical_cast<int>(argv[2]));
	}
	boost::asio::io_service io_service;
	const auto link_db =
		config::CreateLinkDbWithFile(io_service, "links.txt");
	const auto connection_pool = 
		neuria::network::ConnectionPool::Create(io_service, 
			neuria::network::OnConnectionAdded(
					[](const neuria::network::Connection::ConstPtr& connection){
				/*
				const auto node_id = 
					CreateNodeIdFromHostNameAndPortNumber(
						connection->GetRemoteHostName(),
						connection->GetRemotePortNumber()
					);
				*/
			}),
			neuria::network::OnConnectionRemoved());
	const auto search_file_key_hash_db = 
		database::CreateStandardFileKeyHashDb(io_service);
	const auto spread_file_key_hash_db = 
		database::CreateStandardFileKeyHashDb(io_service);
	CuiSyncia cui_syncia(
		io_service,
		host_name, port_number,
		neuria::network::BufferSize(256),
		database::FileSystemPath("./download"),
		link_db,
		connection_pool,
		search_file_key_hash_db,
		spread_file_key_hash_db,
		std::cout,
		std::cout,
		std::cin);
	cui_syncia.InitShell();
	cui_syncia.InitDispatcher();
	cui_syncia.InitTimer();
	boost::thread_group thread_group;
	for(unsigned int i = 0; i < 5; ++i){
		thread_group.create_thread(
			boost::bind(&boost::asio::io_service::run, &io_service)
		);
	}
	cui_syncia.CreateInitialLink();
	cui_syncia.StartAcceptInBackground();
	cui_syncia.StartCuiShell();
	thread_group.join_all();

    return 0;
}

#endif

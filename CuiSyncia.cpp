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
	CuiSyncia cui_syncia(
		io_service,
		host_name, port_number,
		neuria::network::BufferSize(256));
	cui_syncia.InitShell();
	cui_syncia.InitDispatcher();
	cui_syncia.InitTimer();
	cui_syncia.Run();

    return 0;
}

#endif

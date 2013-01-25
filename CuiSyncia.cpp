#ifdef CUISYNCIA_UNIT_TEST
#include "CuiSyncia.h"
#include <iostream>

using namespace syncia;
using namespace neuria::network;

int main(int argc, char* argv[])
{
	CuiSyncia cui_syncia(
		HostName("localhost"), PortNumber(54321),
		neuria::network::BufferSize(256));
	cui_syncia.InitShell();
	cui_syncia.InitDispatcher();
	cui_syncia.Run();

    return 0;
}

#endif

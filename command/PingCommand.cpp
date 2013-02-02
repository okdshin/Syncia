#ifdef PINGCOMMAND_UNIT_TEST
#include "PingCommand.h"
#include <iostream>

using namespace syncia;
using namespace syncia::command;

int main(int argc, char* argv[])
{
	auto command = PingCommand();
	std::cout << command << std::endl;
	std::cout << neuria::command::CreateStringFromByteArray(command.Serialize()) << std::endl;
	auto serialized_byte_array = command.Serialize();
	//serialized_byte_array.erase(serialized_byte_array.begin(), serialized_byte_array.begin()+2);
	std::cout << "parsed:\n" << PingCommand::Parse(serialized_byte_array) << std::endl;

    return 0;
}

#endif

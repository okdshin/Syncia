#ifdef FILEKEYHASHCOMMAND_UNIT_TEST
#include "FileKeyHashCommand.h"
#include <iostream>

using namespace syncia;
using namespace syncia::command;

int main(int argc, char* argv[])
{
	auto command = FileKeyHashCommand(neuria::command::CreateByteArrayFromString("hello"));
	std::cout << command << std::endl;
	std::cout << neuria::command::CreateStringFromByteArray(command.Serialize()) << std::endl;
	auto serialized_byte_array = command.Serialize();
	//serialized_byte_array.erase(serialized_byte_array.begin(), serialized_byte_array.begin()+2);
	std::cout << "parsed:\n" << FileKeyHashCommand::Parse(serialized_byte_array) << std::endl;
	command.SetFileBodyByteArray(neuria::command::CreateByteArrayFromString("test"));
	std::cout << command << std::endl;

    return 0;
}

#endif

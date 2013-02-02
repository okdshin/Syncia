#pragma once
//EchoCommand:20120913
#include <iostream>
#include <sstream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "../Neuria/Neuria.h"

namespace syncia{
namespace command{

class EchoCommand{
public: 
	EchoCommand(){}
	static auto GetRequestCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("echo_command_request");	
	}

	static auto GetReplyCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("echo_command_reply");	
	}

	static auto Parse(const neuria::command::ByteArray& byte_array) -> EchoCommand {
		std::stringstream ss(neuria::command::CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto command = EchoCommand();
		ia >> command;
		return command;
	}
	
	auto Serialize()const -> neuria::command::ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const EchoCommand&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "Error:EchoCommand::Serialize:"<< e.what() << std::endl;	
		}
		return neuria::command::CreateByteArrayFromString(ss.str());	
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){
		ar & byte_array;
	}

public:
	EchoCommand(const neuria::command::ByteArray& byte_array) 
		: byte_array(byte_array){}

	auto GetWrappedByteArray()const -> neuria::command::ByteArray {
		return this->byte_array;	
	}

private:
	neuria::command::ByteArray byte_array;

};

inline auto operator<<(std::ostream& os, const EchoCommand& command) -> std::ostream& {
	os << boost::format("echo:%1%") 
		% neuria::command::CreateStringFromByteArray(command.GetWrappedByteArray());
	return os;
}

}
}

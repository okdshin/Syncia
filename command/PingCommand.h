#pragma once
//PingCommand:20120913
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

class PingCommand{
public: 
	PingCommand(){}

	static auto GetCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("ping_command");	
	}

	static auto Parse(const neuria::command::ByteArray& byte_array) -> PingCommand {
		std::stringstream ss(neuria::command::CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto command = PingCommand();
		ia >> command;
		return command;
	}
	
	auto Serialize()const -> neuria::command::ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const PingCommand&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "Error:PingCommand::Serialize:"<< e.what() << std::endl;	
		}
		return neuria::command::CreateByteArrayFromString(ss.str());	
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){
	}

};

inline auto operator<<(std::ostream& os, const PingCommand& command) -> std::ostream& {
	os << "ping";
	return os;
}

}
}

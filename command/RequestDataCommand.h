#pragma once
//RequestDataCommand:20120913
#include <iostream>
#include <sstream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "ByteArray.h"
#include "CommandId.h"

namespace neuria{
namespace command{

class RequestDataCommand{
public: 
	RequestDataCommand(const ByteArray& wrapped_byte_array){}

	static auto Parse(const ByteArray& byte_array) -> RequestDataCommand {
		std::stringstream ss(CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto command = RequestDataCommand();
		ia >> command;
		return command;
	}
	
	auto Serialize()const -> ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const RequestDataCommand&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "Error:RequestDataCommand::Serialize:"<< e.what() << std::endl;	
		}
		return CreateByteArrayFromString(ss.str());	
	}

	auto Get...() -> ... {
		...	
	}
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){
		ar & ...;
	}

};

inline auto operator<<(std::ostream& os, const RequestDataCommand& command) -> std::ostream& {
	os << ...
	return os;
}

}
}

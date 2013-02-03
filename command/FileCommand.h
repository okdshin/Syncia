#pragma once
//FileCommand:20120913
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

class FileCommand{
public: 
	FileCommand(){}
	static auto GetPullCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_command_pull");	
	}

	static auto GetPushCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_command_push");	
	}

	static auto Parse(const neuria::command::ByteArray& byte_array) -> FileCommand {
		std::stringstream ss(neuria::command::CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto command = FileCommand();
		ia >> command;
		return command;
	}
	
	auto Serialize()const -> neuria::command::ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const FileCommand&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "Error:FileCommand::Serialize:"<< e.what() << std::endl;	
		}
		return neuria::command::CreateByteArrayFromString(ss.str());	
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){
		ar & hash_id_byte_array;
		ar & file_body_byte_array;
	}

public:
	FileCommand(const neuria::command::ByteArray& hash_id_byte_array)
		: hash_id_byte_array(hash_id_byte_array),
		file_body_byte_array(){}

	auto GetHashIdByteArray()const -> neuria::command::ByteArray {
		return this->hash_id_byte_array;	
	}

	auto SetFileBodyByteArray(const neuria::command::ByteArray& byte_array) -> void {
		this->file_body_byte_array = byte_array;	
	} 
	
	auto GetFileBodyByteArray()const -> neuria::command::ByteArray {
		return this->file_body_byte_array;	
	}

private:
	neuria::command::ByteArray hash_id_byte_array;
	neuria::command::ByteArray file_body_byte_array;
};

inline auto operator<<(std::ostream& os, const FileCommand& command) -> std::ostream& {
	os << boost::format("FileCommand:{HashId:\"%1%\", FileBody:\"%2%\"}") 
		% neuria::command::CreateStringFromByteArray(command.GetHashIdByteArray())
		% neuria::command::CreateStringFromByteArray(command.GetFileBodyByteArray());
	return os;
}

}
}

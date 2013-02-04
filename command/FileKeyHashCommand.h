#pragma once
//FileKeyHashCommand:20120913
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

class FileKeyHashCommand{
public: 
	FileKeyHashCommand(){}
	static auto GetFetchPullCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_key_hash_command_fetch_pull");	
	}

	static auto GetFetchPushCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_key_hash_command_fetch_push");	
	}

	static auto GetSpreadPullCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_key_hash_spread_command_pull");	
	}

	static auto GetSpreadPushCommandId() -> neuria::command::CommandId {
		return neuria::command::CommandId("file_key_hash_spread_command_push");	
	}

	static auto Parse(const neuria::command::ByteArray& byte_array) -> FileKeyHashCommand {
		std::stringstream ss(neuria::command::CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto command = FileKeyHashCommand();
		ia >> command;
		return command;
	}
	
	auto Serialize()const -> neuria::command::ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const FileKeyHashCommand&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "Error:FileKeyHashCommand::Serialize:"<< e.what() << std::endl;	
		}
		return neuria::command::CreateByteArrayFromString(ss.str());	
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){
		ar & hash_id_byte_array;
	}

public:
	FileKeyHashCommand(const neuria::command::ByteArray& hash_id_byte_array)
		: hash_id_byte_array(hash_id_byte_array),
		file_body_byte_array(){}

	auto AddFileKeyHashByteArray(const neuria::command::ByteArray& byte_array) -> void {
		this->file_key_hash_byte_array_list.push_back(byte_array);
	}

	auto ForEach(boost::function<(const neuria::command::ByteArray&)> func)const -> void {
		for(const auto file_key_hash_byte_array : this->file_key_hash_byte_array_list){
			func(file_key_hash);
		}
	}

private:
	std::vector<neuria::command::ByteArray> file_key_hash_byte_array_list;
};

inline auto operator<<(std::ostream& os, const FileKeyHashCommand& command) -> std::ostream& {
	os << boost::format("FileKeyHashCommand:{HashId:\"%1%\", FileBody:\"%2%\"}") 
		% neuria::command::CreateStringFromByteArray(command.GetHashIdByteArray())
		% neuria::command::CreateStringFromByteArray(command.GetFileBodyByteArray());
	return os;
}

}
}

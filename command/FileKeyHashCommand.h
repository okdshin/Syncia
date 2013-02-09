#pragma once
//FileKeyHashCommand:20120913
#include <iostream>
#include <algorithm>
#include <sstream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "../Neuria/Neuria.h"
#include "NodeId.h"
#include "Keyword.h"

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
		ar & node_id_on_route_str_list & file_key_hash_byte_array_list;
	}

public:
	FileKeyHashCommand(
			const NodeId& initial_sender_node_id, const Keyword& keyword) 
			: keyword(keyword){
		node_id_on_route_str_list.push_back(initial_sender_node_id.ToString());	
	}

	auto GetInitialSenderNodeId()const -> NodeId {
		return NodeId(this->node_id_on_route_str_list.front());	
	}

	auto GetCurrentHopCount()const -> unsigned int {
		return this->node_id_on_route_str_list.size();	
	}

	auto GetKeyword()const -> Keyword {
		return this->keyword;	
	}

	auto AddNodeIdOnRoute(const NodeId& node_id) -> void {
		this->node_id_on_route_str_list.push_back(node_id.ToString());
	}

	auto GetNextPushNodeId(const NodeId& node_id) -> NodeId {
		const auto found_iter = std::find(
			this->node_id_on_route_str_list.begin(),
			this->node_id_on_route_str_list.end(), 
			node_id.ToString());
		assert("please call and check that this is not initial sender." 
			&& found_iter != this->node_id_on_route_str_list.begin());
		if(found_iter == this->node_id_on_route_str_list.end()){
			std::cout << 
				"strange! anyway return the initial sender node_id" << std::endl;	
			return NodeId(this->node_id_on_route_str_list.front());
		}

		return NodeId(*(found_iter-1));
	}

	auto AddFileKeyHashByteArray(
			const neuria::command::ByteArray& byte_array) -> void {
		this->file_key_hash_byte_array_list.push_back(byte_array);
	}

	auto ForEach(boost::function<void (
			const neuria::command::ByteArray&)> func)const -> void {
		for(const auto file_key_hash_byte_array : this->file_key_hash_byte_array_list){
			func(file_key_hash_byte_array);
		}
	}

private:
	Keyword keyword;
	std::vector<std::string> node_id_on_route_str_list;
	std::vector<neuria::command::ByteArray> file_key_hash_byte_array_list;
};

inline auto operator<<(
		std::ostream& os, const FileKeyHashCommand& command) -> std::ostream& {
	os << "FileKeyHashCommand:: Keyword:\"" 
		<< command.GetKeyword().ToString() << "\", ";
	command.ForEach([&os](const neuria::command::ByteArray& byte_array){
		os << neuria::command::CreateStringFromByteArray(byte_array) << std::endl;	
	});
	return os;
}

}
}

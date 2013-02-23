#pragma once
//FileKeyHash:20120903
#include <iostream>
#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "FileSystemPath.h"
#include "NodeId.h"
#include "Routine.h"

namespace syncia{
namespace database{

inline auto SerializeFile(const FileSystemPath& file_path) -> ByteArray {
	boost::filesystem::ifstream ifs(file_path, std::ios::binary);
	std::stringstream ss;
	ss << ifs.rdbuf();
	const auto byte_array_str = ss.str();
	auto whole_byte_array = CreateByteArrayFromString(byte_array_str);
	std::cout << boost::format("file serialized: %1% bytes") 
		% whole_byte_array.size()
	<< std::endl;

	return whole_byte_array;
}

inline auto ParseFile(const FileSystemPath& file_path, 
		const ByteArray& file_byte_array) -> void {
	boost::filesystem::ofstream ofs(file_path, std::ios::binary);
	ofs.write(static_cast<char*>(static_cast<void*>(const_cast<uint8_t*>(
		&file_byte_array.front()))), file_byte_array.size());
	ofs.close();
}

class FileKeyHash{
public:
   	FileKeyHash(){}
	FileKeyHash(const HashId& hash_id, const Keyword& keyword, 
			const NodeId& owner_id, const FileSystemPath& file_path) 
		:hash_id_str(hash_id.ToString()), 
		keyword_str(keyword.ToString()), 
		owner_id_str(owner_id.ToString()), 
		file_path_str(file_path.string()){
		this->ModifyLastCheckedTimeNow();
	}
	
	FileKeyHash(const NodeId& owner_id, const FileSystemPath& file_path) 
			:hash_id_str(), 
			keyword_str(), 
			owner_id_str(owner_id.ToString()), 
			file_path_str(file_path.string()){
		const auto file_body_byte_array = SerializeFile(file_path);
		const auto hash_id = database::CalcHashId(file_body_byte_array);
		this->hash_id_str = hash_id.ToString();
		this->keyword_str = file_path.filename().string();
		this->ModifyLastCheckedTimeNow();
	}

	auto GetHashId() const -> HashId { 
		return HashId(hash_id_str); 
	}

	auto GetKeyword() const -> Keyword { 
		return Keyword(keyword_str); 
	}

	auto GetOwnerId() const -> NodeId { 
		return NodeId(owner_id_str); 
	}

	auto GetFilePath() const -> FileSystemPath { 
		return FileSystemPath(this->file_path_str); 
	}

	auto GetLastCheckedTime() const -> boost::posix_time::ptime { 
		return boost::posix_time::time_from_string(this->last_checked_time_str); 
	}

	auto ModifyLastCheckedTimeNow() -> void {
		this->last_checked_time_str 
			= boost::posix_time::to_simple_string(
				boost::posix_time::second_clock::universal_time());	
	}
	
	static auto Parse(const ByteArray& byte_array) -> FileKeyHash {
		std::stringstream ss(CreateStringFromByteArray(byte_array));
		boost::archive::text_iarchive ia(ss);
		auto file_key_hash = FileKeyHash();
		ia >> file_key_hash;
		return file_key_hash;
	}
	
	auto Serialize()const -> ByteArray {
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		try{
			oa << static_cast<const FileKeyHash&>(*this);
		}
		catch(const std::exception& e){
			std::cout << "FileKeyHash::Serialize:"<< e.what() << std::endl;	
			throw e;
		}
		return CreateByteArrayFromString(ss.str());	
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, unsigned int ver){	
		ar & hash_id_str & keyword_str & owner_id_str
			& file_path_str & last_checked_time_str;
	}
	
	std::string hash_id_str;
	std::string keyword_str;
	std::string owner_id_str;
	std::string file_path_str;
	std::string last_checked_time_str;
};

inline auto IsFileExist(const FileKeyHash& key_hash) -> bool {
	boost::filesystem::ifstream ifs(key_hash.GetFilePath());
	return static_cast<bool>(ifs);
}

inline auto CreateTestFileKeyHash() -> FileKeyHash {
	return FileKeyHash(
		HashId("test_hash_id"), 
		Keyword("test.txt"),
		NodeId("test_node_id"),
		FileSystemPath("test.txt")
	);
}

inline auto operator<<(std::ostream& os, const FileKeyHash& key_hash) -> std::ostream& {
	os << boost::format(
		"Keyword:%1%, HashId:%2%, OwnerId:%3%, FilePath:%4%, BirthUniversalTime:%5%") 
		% key_hash.GetKeyword().ToString() 
		% key_hash.GetHashId().ToString()
		% key_hash.GetOwnerId().ToString()
		% key_hash.GetFilePath().string()
		% boost::posix_time::to_simple_string(key_hash.GetLastCheckedTime());
	return os;
}

inline auto IsSameHashId(const FileKeyHash& left, const FileKeyHash& right) -> bool {
	return left.GetHashId() == right.GetHashId();	
}

using FileKeyHashList = std::vector<FileKeyHash>;
inline auto operator<<(std::ostream& os, 
		const FileKeyHashList& key_hash_list) -> std::ostream& {
	for(const auto& key_hash : key_hash_list){
		os << key_hash << "\n";
	}
	return os;
}


}
}

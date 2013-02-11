#pragma once
//FILEKEYHASHDB:20120831
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <iterator>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "FileKeyHash.h"

namespace syncia{
namespace database{


using FileKeyHashList = std::vector<FileKeyHash>;
inline auto operator<<(std::ostream& os, 
		const FileKeyHashList& key_hash_list) -> std::ostream& {
	for(const auto& key_hash : key_hash_list){
		os << key_hash << "\n";
	}
	return os;
}

class FileKeyHashDb{
public:
	FileKeyHashDb() : hash_list(){}

	auto Add(const FileKeyHash& key_hash) -> void {
		std::cout << "add outer" << std::endl;
		auto is_equal_to_key_hash = 
			[&key_hash](const FileKeyHash& e) -> bool {
				return key_hash.GetHashId() == e.GetHashId()
					&& key_hash.GetOwnerId() == e.GetOwnerId(); };

		// when same owner_id and hash_id has already existed.
		if(std::find_if(this->hash_list.begin(), this->hash_list.end(), 
				is_equal_to_key_hash) != this->hash_list.end()){
			std::replace_if(this->hash_list.begin(), this->hash_list.end(), 
				is_equal_to_key_hash, key_hash);
		}
		else{
			std::cout << "add inner" << std::endl;
			this->hash_list.push_back(key_hash);
		}
	}

	auto Search(const Keyword& keyword) -> FileKeyHashList {
		std::vector<std::string> keyword_str_list;
		auto raw_keyword_str = keyword.ToString();
		boost::algorithm::split(
			keyword_str_list, raw_keyword_str, boost::is_any_of(" "));
		//AND search
		auto end = std::partition(this->hash_list.begin(), this->hash_list.end(), 
			[&keyword_str_list](const FileKeyHash& key_hash){
				for(const auto& keyword_str : keyword_str_list){
					if(!boost::contains(
							key_hash.GetKeyword().ToString(), keyword_str)){
						return false;
					}
				}
				return true;
			});
		return FileKeyHashList(this->hash_list.begin(), end);
	}

	auto GetByHashId(const HashId& hash_id) -> FileKeyHash {
		auto found = std::find_if(this->hash_list.begin(), this->hash_list.end(), 
			[&hash_id](const FileKeyHash& key_hash){
				return key_hash.GetHashId() == hash_id; });	
		if(found == this->hash_list.end()){
			assert(!"not found");	
		}
		return *found;
	}
	
	auto GetByIndex(unsigned int index) -> FileKeyHash {
		return this->hash_list.at(index);	
	}

	auto IsContain(const HashId& hash_id) -> bool {
		auto found = std::find_if(this->hash_list.begin(), this->hash_list.end(), 
			[&hash_id](const FileKeyHash& key_hash){
				return key_hash.GetHashId() == hash_id; });	
		return found != this->hash_list.end();
	}

	auto GetNewer(unsigned int max_count) -> FileKeyHashList {
		if(max_count > this->hash_list.size()){
			return this->hash_list;	
		}

		std::nth_element(this->hash_list.begin(), 
			this->hash_list.begin()+max_count, 
			this->hash_list.end(), 
			[](const FileKeyHash& left, const FileKeyHash& right){
				return left.GetLastCheckedTime() > right.GetLastCheckedTime();	
			});
		auto newer_list = FileKeyHashList();
		std::copy(this->hash_list.begin(), 
			this->hash_list.begin()+max_count, std::back_inserter(newer_list));
		return newer_list;
	}

	auto Clear() -> void {
		this->hash_list.clear();	
	}

private:
	friend auto operator<<(std::ostream& os, 
		const FileKeyHashDb& file_key_hash_db) -> std::ostream&;

	FileKeyHashList hash_list;
};

inline auto operator<<(std::ostream& os, 
		const FileKeyHashDb& file_key_hash_db) -> std::ostream& {
	for(unsigned int i = 0; i < file_key_hash_db.hash_list.size(); ++i){
		os << "[" << i << "]:" << file_key_hash_db.hash_list.at(i) << "\n";
	}

	return os;
}

}
}

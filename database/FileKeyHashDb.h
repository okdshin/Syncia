#pragma once
//FILEKEYHASHDB:20120831
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <iterator>
#include <algorithm>
#include <boost/format.hpp>
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
	//using ApplyFunc = boost::function<void (FileKeyHash&)>;
	//using IsErasedDecider = boost::function<bool (const FileKeyHash&)>;

	FileKeyHashDb() : hash_list(){}

	auto Add(const FileKeyHash& key_hash) -> void {
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
			this->hash_list.push_back(key_hash);
		}
	}

	/*
	auto Erase(IsErasedDecider decider) -> void {
		this->hash_list.erase(
			std::remove_if(this->hash_list.begin(), this->hash_list.end(), 
				[decider](const FileKeyHash& key_hash) -> bool {
					return decider(key_hash)();
				}),
			this->hash_list.end()
		);
	}
	auto Search(const KeywordList& search_keyword_list) -> FileKeyHashList {
		for(const auto& key_hash : this->hash_list){
			this->os << key_hash.GetKeyword() << "|";
			for(const auto& keyword : search_keyword_list()){
				this->os << keyword << " ";	
			}
		}
		auto end = std::partition(this->hash_list.begin(), this->hash_list.end(), 
			[&](const FileKeyHash& key_hash){
				return CalcSimilarity(
					search_keyword_list, key_hash.GetKeyword()) > this->threshold;	
			});
		return FileKeyHashList(this->hash_list.begin(), end);
	}
	*/

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

	/*
	auto GetFileKeyHashList() -> FileKeyHashList {
		return this->hash_list;	
	}

	auto Erase(const HashId& hash_id) -> void {
		this->hash_list.erase(
			std::remove_if(this->hash_list.begin(), this->hash_list.end(), 
				[&hash_id](const FileKeyHash& key_hash){
					return key_hash.GetHashId()() == hash_id(); }), 
			this->hash_list.end());	
	}

	auto Erase(const neuria::ByteArray& src_data) -> void {
		this->hash_list.erase(
			std::remove_if(this->hash_list.begin(), this->hash_list.end(), 
				[&src_data](const FileKeyHash& key_hash)
					{ return key_hash.GetHashId()() == CalcHashId(src_data)(); }), 
			this->hash_list.end());	
	}

	auto Erase(const FileSystemPath& file_path) -> void {
		this->hash_list.erase(
			std::remove_if(this->hash_list.begin(), this->hash_list.end(), 
				[&file_path](const FileKeyHash& key_hash)
					{ return key_hash.GetFilePath() == file_path; }), 
			this->hash_list.end());		
	}
	*/
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

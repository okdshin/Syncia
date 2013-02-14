#pragma once
//FILEKEYHASHDB:20120831
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <iterator>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
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
	FileKeyHashDb(boost::asio::io_service& io_service) 
		: strand(new boost::asio::io_service::strand(io_service)), hash_list(){}

	auto Add(const FileKeyHash& key_hash) -> void {
		this->strand->post([this, key_hash](){
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
		});
	}

	auto Remove(boost::function<bool (const FileKeyHash&)> decider) -> void {
		this->strand->post([this, decider](){
			const auto new_end = 
				std::remove_if(
					this->hash_list.begin(), this->hash_list.end(), decider);
			this->hash_list.erase(new_end, this->hash_list.end());
		});
	}

	auto QuoteSearch(
			const Keyword& keyword, 
			boost::function<void (const FileKeyHashList&)> func)const -> void {
		this->strand->post([this, keyword, func](){
			std::vector<std::string> keyword_str_list;
			auto raw_keyword_str = keyword.ToString();
			boost::algorithm::split(
				keyword_str_list, raw_keyword_str, boost::is_any_of(" "));
			//AND search
			auto temp_hash_list = this->hash_list;
			auto end = std::partition(temp_hash_list.begin(), temp_hash_list.end(), 
				[&keyword_str_list](const FileKeyHash& key_hash){
					for(const auto& keyword_str : keyword_str_list){
						if(!boost::contains(
								key_hash.GetKeyword().ToString(), keyword_str)){
							return false;
						}
					}
					return true;
				});
			func(FileKeyHashList(temp_hash_list.begin(), end));
		});
	}

	auto QuoteByHashId(
			const HashId& hash_id, 
			boost::function<void (const FileKeyHash&)> func)const -> void {
		this->strand->post([this, hash_id, func](){
			auto found = std::find_if(this->hash_list.begin(), this->hash_list.end(), 
				[&hash_id](const FileKeyHash& key_hash){
					return key_hash.GetHashId() == hash_id; });	
			if(found == this->hash_list.end()){
				assert(!"not found");	
			}
			func(*found);
		});
	}
	
	auto QuoteByIndex(
			unsigned int index, 
			boost::function<void (const FileKeyHash&)> func)const -> void {
		this->strand->post([this, index, func](){
			func(this->hash_list.at(index));	
		});
	}

	auto AskIsContain(
			const HashId& hash_id,
			boost::function<void (bool)> func)const -> void {
		this->strand->post([this, hash_id, func](){
			auto found = std::find_if(this->hash_list.begin(), this->hash_list.end(), 
				[&hash_id](const FileKeyHash& key_hash){
					return key_hash.GetHashId() == hash_id; });	
			func(found != this->hash_list.end());
		});
	}

	auto QuoteNewerList(
			unsigned int max_count, 
			boost::function<void (const FileKeyHashList&)> func)const -> void {
		this->strand->post([this, max_count, func](){
			if(max_count > this->hash_list.size()){
				func(this->hash_list);
				return;
			}

			auto temp_hash_list = this->hash_list;
			std::nth_element(temp_hash_list.begin(), 
				temp_hash_list.begin()+max_count, 
				temp_hash_list.end(), 
				[](const FileKeyHash& left, const FileKeyHash& right){
					return left.GetLastCheckedTime() > right.GetLastCheckedTime();	
				});
			temp_hash_list.erase(temp_hash_list.begin(), 
				temp_hash_list.begin()+max_count);
			func(temp_hash_list);
		});
	}

private:
	friend auto operator<<(std::ostream& os, 
		const FileKeyHashDb& file_key_hash_db) -> std::ostream&;

	boost::scoped_ptr<boost::asio::io_service::strand> strand;
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

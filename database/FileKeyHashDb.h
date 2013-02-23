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
#include "FileKeyHashAdder.h"
#include "FileKeyHashRemover.h"
#include "FileKeyHashListQuoter.h"

namespace syncia{
namespace database{


class FileKeyHashDb{
public:
	using Ptr = boost::shared_ptr<FileKeyHashDb>;

	static auto Create(boost::asio::io_service& io_service,
		const FileKeyHashAdder& adder,
		const FileKeyHashRemover& remover,
		const FileKeyHashListQuoter& quoter) -> Ptr {
		return Ptr(new FileKeyHashDb(io_service, adder, remover, quoter));
	}

	auto Add(const FileKeyHash& key_hash) -> void {
		this->strand->post([this, key_hash](){
			this->adder(key_hash);
		});
	}

	auto Remove(boost::function<bool (const FileKeyHash&)> decider) -> void {
		this->strand->post([this, decider](){
			this->remover(decider);
		});
	}

	auto QuoteSearch(
			const Keyword& keyword, 
			boost::function<void (const FileKeyHashList&)> func)const -> void {
		this->strand->post([this, keyword, func](){
			this->quoter([keyword, func](const FileKeyHashList& file_key_hash_list){
				std::vector<std::string> keyword_str_list;
				auto raw_keyword_str = keyword.ToString();
				boost::algorithm::split(
					keyword_str_list, raw_keyword_str, boost::is_any_of(" "));
				//AND search
				auto temp_hash_list = file_key_hash_list;
				auto end = std::remove_if(temp_hash_list.begin(), temp_hash_list.end(), 
					[&keyword_str_list](const FileKeyHash& key_hash){
						for(const auto& keyword_str : keyword_str_list){
							if(!boost::contains(
									key_hash.GetKeyword().ToString(), keyword_str)){
								return false;
							}
						}
						return true;
					});
				temp_hash_list.erase(temp_hash_list.begin(), end);
				func(temp_hash_list);	
			});
		});
	}

	auto QuoteByHashId(
			const HashId& hash_id, 
			boost::function<void (const FileKeyHashList&)> func)const -> void {
		this->strand->post([this, hash_id, func](){
			this->quoter([hash_id, func](const FileKeyHashList& file_key_hash_list){
				FileKeyHashList found_list;
				for(const auto& file_key_hash : file_key_hash_list){
					if(file_key_hash.GetHashId() == hash_id){
						found_list.push_back(file_key_hash);	
					}
				}
				func(found_list);
			});
		});
	}
	
	auto QuoteNewerList(
			unsigned int max_count, 
			boost::function<void (const FileKeyHashList&)> func)const -> void {
		this->strand->post([this, max_count, func](){
			this->quoter([max_count, func](const FileKeyHashList& file_key_hash_list){
				
				if(max_count > file_key_hash_list.size()){
					func(file_key_hash_list);
					return;
				}

				auto temp_hash_list = file_key_hash_list;
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
		});
	}

private:
	FileKeyHashDb(boost::asio::io_service& io_service,
		const FileKeyHashAdder& adder,
		const FileKeyHashRemover& remover,
		const FileKeyHashListQuoter& quoter) 
		: strand(new boost::asio::io_service::strand(io_service)), 
		adder(adder), remover(remover), quoter(quoter){}

	friend auto operator<<(std::ostream& os, 
		const FileKeyHashDb& file_key_hash_db) -> std::ostream&;

	boost::scoped_ptr<boost::asio::io_service::strand> strand;
	FileKeyHashAdder adder;
	FileKeyHashRemover remover;
	FileKeyHashListQuoter quoter;
};

inline auto CreateBasicFileKeyHashDb(boost::asio::io_service& io_service, 
		const FileKeyHashAdder& arg_adder,
		const FileKeyHashRemover& arg_remover) -> FileKeyHashDb::Ptr {
	auto file_key_hash_list = 
		boost::shared_ptr<FileKeyHashList>(new FileKeyHashList());
	
	const auto adder = 
		FileKeyHashAdder([file_key_hash_list](const FileKeyHash& file_key_hash){
			/*
			const auto iter = 
				find_if(file_key_hash_list->begin(), file_key_hash_list->end(),
					[&file_key_hash](const FileKeyHash& e){	
						return e.GetHashId() == file_key_hash.GetHashId() 
						&& e.GetOwnerId() == file_key_hash.GetOwnerId();
					}
				);
			if(iter != file_key_hash_list->end()){
				file_key_hash_list->erase(iter);
			}
			*/
			file_key_hash_list->push_back(file_key_hash);
			arg_adder(file_key_hash);
		});

	const auto remover = 
		FileKeyHashRemover(
				[file_key_hash_list](boost::function<bool (const FileKeyHash&)> decider){
			const auto end = 
				std::remove_if(
					file_key_hash_list->begin(), 
					file_key_hash_list->end(),
					decider);
			file_key_hash_list->erase(end, file_key_hash_list->end());
			remover(decider);
		});
	
	const auto quoter = 
		FileKeyHashListQuoter([file_key_hash_list](
				boost::function<void (const FileKeyHashList&)> receiver){
			receiver(*file_key_hash_list);	
		});
	return FileKeyHashDb::Create(io_service, adder, remover, quoter);	
}

inline auto CreateBasicFileKeyHashDb(boost::asio::io_service& io_service) -> FileKeyHashDb::Ptr {
	auto file_key_hash_list = 
		boost::shared_ptr<FileKeyHashList>(new FileKeyHashList());
	
	const auto adder = 
		FileKeyHashAdder([file_key_hash_list](const FileKeyHash& file_key_hash){
			const auto iter = 
				find_if(file_key_hash_list->begin(), file_key_hash_list->end(),
					[&file_key_hash](const FileKeyHash& e){	
						return e.GetHashId() == file_key_hash.GetHashId() 
						&& e.GetOwnerId() == file_key_hash.GetOwnerId();
					}
				);
			if(iter != file_key_hash_list->end()){
				file_key_hash_list->erase(iter);
			}
			file_key_hash_list->push_back(file_key_hash);
		});

	const auto remover = 
		FileKeyHashRemover(
				[file_key_hash_list](boost::function<bool (const FileKeyHash&)> decider){
			const auto end = 
				std::remove_if(
					file_key_hash_list->begin(), 
					file_key_hash_list->end(),
					decider);
			file_key_hash_list->erase(end, file_key_hash_list->end());
		});
	
	const auto quoter = 
		FileKeyHashListQuoter([file_key_hash_list](
				boost::function<void (const FileKeyHashList&)> receiver){
			receiver(*file_key_hash_list);	
		});
	return FileKeyHashDb::Create(io_service, adder, remover, quoter);	
}
inline auto operator<<(std::ostream& os, 
		const FileKeyHashDb& file_key_hash_db) -> std::ostream& {
	file_key_hash_db.quoter([&file_key_hash_db, &os](const FileKeyHashList& file_key_hash_list){
		for(unsigned int i = 0; i < file_key_hash_list.size(); ++i){
			os << "[" << i << "]:" << file_key_hash_list.at(i) << "\n";
		}	
	});

	return os;
}

}
}

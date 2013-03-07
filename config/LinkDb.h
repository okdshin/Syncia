#pragma once
//LinkDb:20130304
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include "LinkAdder.h"
#include "LinkRemover.h"
#include "LinkListQuoter.h"

namespace syncia{
namespace config
{
class LinkDb{
public:
	using Ptr = boost::shared_ptr<LinkDb>;
	static auto Create(
			boost::asio::io_service& io_service, 
			const LinkAdder& adder,
			const LinkRemover& remover,
			const LinkListQuoter& quoter) -> Ptr {
		return Ptr(new LinkDb(io_service, adder, remover, quoter));	
	}

	auto Add(const Link& link)const -> void {
		this->strand->post([this, link](){
			this->adder(link);	
		});
	}

	auto RemoveByIndex(unsigned int index)const -> void {
		this->strand->post([this, index](){
			this->remover(index);
		});
	}

	auto QuoteLinkList(boost::function<void (const LinkList&)> receiver)const -> void {
		this->strand->post([this, receiver](){
			this->quoter(receiver);
		});
	}

	auto Save() -> void {
		//todo		
	}

private:
    LinkDb(
			boost::asio::io_service& io_service, 
			const LinkAdder& adder,
			const LinkRemover& remover,
			const LinkListQuoter& quoter)
		: strand(new boost::asio::io_service::strand(io_service)),
		adder(adder), remover(remover), quoter(quoter){}

	const boost::scoped_ptr<boost::asio::io_service::strand> strand;
	const LinkAdder adder;
	const LinkRemover remover;
	const LinkListQuoter quoter;
};

inline auto operator<<(std::ostream& os, const LinkDb& link_db) -> std::ostream& {
	link_db.QuoteLinkList([&os](const LinkList& link_list){
		for(const auto link : link_list){
			os << link.GetNodeId().ToString() << std::endl;
		}
	});
	return os;
}

inline auto CreateBasicLinkDb(
		boost::asio::io_service& io_service) -> LinkDb::Ptr{
	auto link_list = new LinkList();
	auto adder = LinkAdder([link_list](const Link& link){
			link_list->push_back(link);	
		});

	auto remover = LinkRemover([link_list](unsigned int index){
			link_list->erase(link_list->begin()+index);
		});

	auto quoter = LinkListQuoter([link_list](boost::function<void (const LinkList&)> receiver){
			receiver(*link_list);
		});

	return LinkDb::Create(io_service, adder, remover, quoter);
}

inline auto CreateLinkDbWithFile(
		boost::asio::io_service& io_service,
		const std::string& file_name) -> LinkDb::Ptr {
	auto link_config_file = 
		boost::shared_ptr<std::fstream>(new std::fstream(file_name.c_str()),
			[](std::fstream* f){
				std::cout << "saved!" << std::endl;
				f->close();
			}
		);
	auto link_list = 
		boost::shared_ptr<LinkList>(new LinkList());
	std::string line;
	while(*link_config_file && getline(*link_config_file, line)){
		link_list->push_back(Link(NodeId(line)));	
	}
	auto adder = LinkAdder([link_config_file, link_list](const Link& link){
			link_list->push_back(link);	
			*link_config_file << link.GetNodeId().ToString() << std::endl;
		});

	auto remover = LinkRemover([link_list](unsigned int index){
			link_list->erase(link_list->begin()+index);
		});

	auto quoter = LinkListQuoter([link_list](boost::function<void (const LinkList&)> receiver){
			receiver(*link_list);
		});

	return LinkDb::Create(io_service, adder, remover, quoter);	
}

}
}

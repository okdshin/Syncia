#pragma once
//LinkDb:20130304
#include <iostream>
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

	auto Remove(const Link& link)const -> void {
		this->strand->post([this, link](){
			this->remover(link);
		});
	}

	auto QuoteLinkList(boost::function<void (const LinkList&)> receiver)const -> void {
		this->strand->post([this, receiver](){
			this->quoter(receiver);
		});
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

inline auto operator<<(std::ostream& os, const LinkDb::Ptr& link_db) -> std::ostream& {
	link_db->QuoteLinkList([&os](const LinkList& link_list){
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

	auto remover = LinkRemover([link_list](const Link& link){
			const auto new_end = 
				std::remove(link_list->begin(), link_list->end(), link);
			link_list->erase(new_end, link_list->end());
		});

	auto quoter = LinkListQuoter([link_list](boost::function<void (const LinkList&)> receiver){
			receiver(*link_list);
		});

	return LinkDb::Create(io_service, adder, remover, quoter);
}
}
}

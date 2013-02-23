#pragma once
//FileKeyHashListQuoter:20130112
#include <iostream>
#include <boost/function.hpp>
#include "FileKeyHash.h"

namespace syncia{
namespace database
{
class FileKeyHashListQuoter{
private:
	using Receiver = boost::function<void (const FileKeyHashList&)>;
	using FuncType = boost::function<void (Receiver)>;
public:
	FileKeyHashListQuoter()
		:quoter([](Receiver /*receiver*/){
			std::cout << "Called(Default)FileKeyHashListQuoter" << std::endl;
		}){}

    FileKeyHashListQuoter(FuncType quoter)
		: quoter(quoter){}

	auto operator()(Receiver receiver)const -> void {
		quoter(receiver);	
	}
private:
	const FuncType quoter;
};
}
}

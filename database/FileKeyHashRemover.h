#pragma once
//FileKeyHashRemover:20130112
#include <iostream>
#include <boost/function.hpp>
#include "FileKeyHash.h"

namespace syncia{
namespace database
{
class FileKeyHashRemover{
private:
	using Decider = boost::function<bool (const FileKeyHash&)>;
	using FuncType = boost::function<void (Decider)>;
public:
	FileKeyHashRemover()
		:remover([](Decider /*decider*/){
			std::cout << "Called(Default)FileKeyHashRemover" << std::endl;
		}){}

    FileKeyHashRemover(FuncType remover)
		: remover(remover){}

	auto operator()(Decider decider) -> void {
		remover(decider);
	}
private:
	FuncType remover;
};
}
}

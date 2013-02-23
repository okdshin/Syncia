#pragma once
//FileKeyHashAdder:20130112
#include <iostream>
#include <boost/function.hpp>
#include "FileKeyHash.h"

namespace syncia{
namespace database
{
class FileKeyHashAdder{
private:
	using FuncType = boost::function<void (const FileKeyHash&)>;
public:
	FileKeyHashAdder()
		:adder([](const FileKeyHash&){
			std::cout << "Called(Default)FileKeyHashAdder" << std::endl;
		}){}

    FileKeyHashAdder(FuncType adder)
		: adder(adder){}

	auto operator()(const FileKeyHash& file_key_hash) -> void {
		adder(file_key_hash);	
	}
private:
	FuncType adder;
};
}
}

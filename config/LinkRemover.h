#pragma once
//LinkRemover:20130304
#include <iostream>
#include <boost/function.hpp>
#include "Link.h"

namespace syncia{
namespace config
{
class LinkRemover{
	using FuncType = boost::function<void (unsigned int index)>;
public:
    LinkRemover(FuncType func) : func(func){}

	auto operator()(unsigned int index)const -> void {
		func(index);
	}

private:
	const FuncType func;

};
}
}

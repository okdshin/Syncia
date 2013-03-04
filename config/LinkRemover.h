#pragma once
//LinkRemover:20130304
#include <iostream>
#include <boost/function.hpp>
#include "Link.h"

namespace syncia{
namespace config
{
class LinkRemover{
	using FuncType = boost::function<void (const Link&)>;
public:
    LinkRemover(FuncType func) : func(func){}

	auto operator()(const Link& link)const -> void {
		func(link);
	}

private:
	const FuncType func;

};
}
}

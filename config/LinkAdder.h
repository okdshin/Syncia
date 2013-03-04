#pragma once
//LinkAdder:20130304
#include <iostream>
#include <boost/function.hpp>
#include "Link.h"

namespace syncia{
namespace config
{
class LinkAdder{
	using FuncType = boost::function<void (const Link&)>;
public:
    LinkAdder(FuncType func) : func(func){}

	auto operator()(const Link& link)const -> void {
		func(link);
	}

private:
	const FuncType func;

};
}
}

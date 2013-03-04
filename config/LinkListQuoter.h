#pragma once
//LinkListQuoter:20130304
#include <iostream>
#include <boost/function.hpp>
#include "Link.h"

namespace syncia{
namespace config
{
class LinkListQuoter{
	using FuncType = boost::function<void (boost::function<void (const LinkList&)>)>;
public:
    LinkListQuoter(FuncType func) : func(func){}

	auto operator()(boost::function<void (const LinkList&)> receiver)const -> void {
		func(receiver);
	}

private:
	const FuncType func;

};
}
}

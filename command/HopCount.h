#pragma once
//HopCount:20130110
#include <iostream>
#include <boost/lexical_cast.hpp>

namespace syncia{
namespace command
{
class HopCount{
public:
    HopCount(unsigned int hop_count) : hop_count(hop_count){}
    HopCount(const std::string& hop_count_str) 
		: hop_count(boost::lexical_cast<unsigned int>(hop_count_str)){}

	auto ToInt()const -> unsigned int {
		return this->hop_count;	
	}
	
private:
	unsigned int hop_count;
};

auto operator<(const HopCount& left, const HopCount& right) -> bool {
	return left.ToInt() < right.ToInt();
}

auto operator<<(std::ostream& os, const HopCount& hop_count) -> std::ostream& {
	os << "HopCount:" << hop_count.ToInt();
	return os;
}
}
}

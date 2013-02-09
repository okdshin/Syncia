#pragma once
//Keyword:20130202
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp> 

namespace syncia{
namespace command
{
class Keyword{
public:
	Keyword(){}
    Keyword(const std::string& keyword_str) : keyword_str(keyword_str){}
	
	auto ToString()const -> std::string {
		return this->keyword_str;	
	}

	auto IsContain(const Keyword& keyword) -> bool {
		return boost::contains(this->keyword_str, keyword.ToString()); 
	}

private:
	std::string keyword_str;

};

}
}


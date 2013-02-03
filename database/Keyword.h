#pragma once
//Keyword:20130202
#include <iostream>
#include <string>

namespace syncia{
namespace database
{
class Keyword{
public:
    Keyword(const std::string& keyword_str) : keyword_str(keyword_str){}
	
	auto ToString()const -> std::string {
		return this->keyword_str;	
	}

private:
	std::string keyword_str;

};
}
}


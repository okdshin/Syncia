#pragma once
//ByteArray:20130110
#include <iostream>
#include <vector>
#include <string>

namespace syncia{
namespace database
{
using ByteArray = std::vector<uint8_t>;
inline auto CreateByteArrayFromString(const std::string& str) -> ByteArray {
	return ByteArray(str.begin(), str.end());
}
inline auto CreateStringFromByteArray(const ByteArray& byte_array) -> std::string {
	return std::string(byte_array.begin(), byte_array.end());
}
inline auto OutputByteArray(std::ostream& os, const ByteArray& byte_array) -> std::ostream& {
	for(const auto byte : byte_array){
		os << static_cast<int>(byte) << "\t";
	}	
	return os;
}
/*
class ByteArray{
public:
    ByteArray(const std::vector<unsigned char>& raw_byte_array) 
		: raw_byte_array(raw_byte_array){}
    ~ByteArray(){}

	auto GetRawByteArray()const -> std::vector<unsigned char> {
		return this->raw_byte_array;	
	}

private:
	std::vector<unsigned char> raw_byte_array;
};
*/
}
}

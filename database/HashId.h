#pragma once
//HashId:20130202
#include <iostream>
#include <string>
#include "../Neuria/Neuria.h"
#include "ByteArray.h"

namespace syncia{
namespace database
{
class HashId{
public:
    HashId(const std::string& hash_id_str) : hash_id_str(hash_id_str){}
	
	auto ToString()const -> std::string {
		return this->hash_id_str;	
	}

	static auto Parse(const ByteArray& byte_array) -> HashId {
		return HashId(CreateStringFromByteArray(byte_array));
	}

	auto Serialize()const -> ByteArray {
		return CreateByteArrayFromString(this->hash_id_str);
	}

private:
	std::string hash_id_str;

};

inline auto CalcHashId(const ByteArray& byte_array) -> HashId {
	return HashId(neuria::hash::CalcHashStr(byte_array));
}

inline auto operator==(const HashId& left, const HashId& right) -> bool {
	return left.ToString() == right.ToString();
}
}
}


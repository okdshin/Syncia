#pragma once
//SpreadFileKeyHashDb:20130224
#include <iostream>
#include "database/DataBase.h"

namespace syncia
{
class SpreadFileKeyHashDb{
public:
    SpreadFileKeyHashDb(const database::FileKeyHashDb::Ptr& search_file_key_hash_db)
		: search_file_key_hash_db(search_file_key_hash_db){}

	auto operator()()const -> database::FileKeyHashDb::Ptr {
		return search_file_key_hash_db;
	}

private:
	const database::FileKeyHashDb::Ptr search_file_key_hash_db;

};
}


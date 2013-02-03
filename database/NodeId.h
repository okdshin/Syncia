#pragma once
//NodeId:20130202
#include <iostream>
#include <string>

namespace syncia{
namespace database
{
class NodeId{
public:
    NodeId(const std::string& node_id_str) : node_id_str(node_id_str){}
	
	auto ToString()const -> std::string {
		return this->node_id_str;	
	}

private:
	std::string node_id_str;
};

inline auto operator==(const NodeId& left, const NodeId& right) -> bool {
	return left.ToString() == right.ToString();
}

}
}


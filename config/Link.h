#pragma once
//Link:20130305
#include <iostream>
#include <vector>
#include "NodeId.h"

namespace syncia{
namespace config
{
class Link{
public:
    Link(const NodeId& node_id) : node_id(node_id){}

	auto GetNodeId()const -> NodeId {
		return this->node_id;	
	}

private:
	NodeId node_id;
};

inline auto operator==(const Link& left, const Link& right) -> bool {
	return left.GetNodeId() == right.GetNodeId();	
}

using LinkList = std::vector<Link>;
}
}


#pragma once
//FileSystemPathList:20121002
#include <iostream>
#include <boost/filesystem.hpp>
#include <cassert>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <deque>
#include <string>

namespace syncia {
namespace database {
using FileSystemPath = boost::filesystem::path;

inline auto CreateNecessaryDirectory(const FileSystemPath& file_path) -> void {
	assert(!boost::filesystem::is_directory(file_path));
	boost::filesystem::create_directories(file_path.parent_path());
}

inline auto NormalizeAndExtractRooterPath(const FileSystemPath& path) -> FileSystemPath {
	std::string path_str = path.generic_string();
	std::vector<std::string> item_list;
	boost::algorithm::split(item_list, path_str, boost::is_any_of("/"));
	std::deque<std::string> normalized_list;
	for(const auto& item : item_list){
		if(item.empty()){
		}else
		if(item == "."){
		}else
		if(item == ".."){
			if(!normalized_list.empty()){
				normalized_list.pop_back();
			}
		}
		else{
			normalized_list.push_back(item);	
		}

	}
	std::string integrated;
	normalized_list.pop_front();
	for(const auto& item : normalized_list){
		integrated = integrated + "/" + item;
	}
	integrated.erase(integrated.begin());
	return FileSystemPath(integrated);
}
}
}

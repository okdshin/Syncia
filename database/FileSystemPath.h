#pragma once
//FileSystemPathList:20121002
#include <iostream>
#include <boost/filesystem.hpp>
#include <cassert>

namespace syncia {
namespace database {
using FileSystemPath = boost::filesystem::path;

inline auto CreateNecessaryDirectory(const FileSystemPath& file_path) -> void {
	assert(!boost::filesystem::is_directory(file_path));
	boost::filesystem::create_directories(file_path.parent_path());
}

}
}

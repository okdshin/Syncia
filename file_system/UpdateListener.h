#pragma once
//UpdateListener:20130202
#include <iostream>
#include <FileWatcher/FileWatcher.h>

namespace file_system
{
class UpdateListener : public FW::FileWatchListener {
public:
    UpdateListener(){}

	auto RegisterFunc() -> void {
		
	}
private:

};
}


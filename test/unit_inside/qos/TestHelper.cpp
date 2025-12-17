#include "TestHelper.h"
#include <iostream>

bool TestHelper::waitmatch(DataWriter* writer, unsigned int count, int timeout) {
	int i = 0;
	while (i < timeout) {
		InstanceHandleSeq handles;
		ReturnCode_t ret = writer->get_matched_subscriptions(handles);
		if (ret == TRAVODDS::RETCODE_OK) {
			if (handles.size() >= count) {
				return true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		++i;
	}
	return false;
}

bool TestHelper::waitmatch(DataReader* reader, unsigned int count, int timeout) {
	int i = 0;
	while (i < timeout) {
		InstanceHandleSeq handles;
		ReturnCode_t ret = reader->get_matched_publications(handles);
		if (ret == TRAVODDS::RETCODE_OK) {
			if (handles.size() >= count) {
				return true;
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
		++i;
	}
	return false;
}
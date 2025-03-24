#include "RTPS/WriterProxy.h"

USING_AGIDDS_NAMESPACE

WriterProxy::WriterProxy(GUID_t guid, LocatorSeq & unicastLocatorList, LocatorSeq & multicastLocatorList)
{
	remoteWriterGuid_ = guid;
	unicastLocatorList_ = unicastLocatorList;
	multicastLocatorList_ = multicastLocatorList;
}

GUID_t WriterProxy::GetGuid()
{
	return remoteWriterGuid_;
}

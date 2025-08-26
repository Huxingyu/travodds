#include "rtps/transport/asiocontext.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;

AsioContext::AsioContext()
	: ioContext_(), workGuard_(ioContext_.get_executor())
{
	ioThread_ = std::thread([this]() {
		try {
			ioContext_.run();
		} catch (const std::exception& e) {
			static_cast<void>(e);
			TRAVODDS_LOG(LOG_ERROR, "ioContext Err e[%s]", e.what());
		}
	});
}

AsioContext& AsioContext::GetInstance()
{
	static AsioContext instance;
	return instance;
}

AsioContext::~AsioContext()
{
	workGuard_.reset();
	ioContext_.stop();
	if (ioThread_.joinable()) {
		ioThread_.join();
	}
}

asio::io_context& AsioContext::GetIOContext()
{
	return GetInstance().ioContext_;
}


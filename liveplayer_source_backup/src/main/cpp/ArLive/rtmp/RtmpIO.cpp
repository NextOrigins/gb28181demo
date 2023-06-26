#include "RtmpIO.h"
#include "include/aio-socket.h"
#include "libaio/include/aio-timeout.h"
#include "../../webrtc/rtc_base/internal/default_socket_server.h"

RtmpIO&RtmpIO::Inst()
{
	static RtmpIO gInst;
	return gInst;
}

RtmpIO::RtmpIO(void)
	: rtc::Thread(rtc::CreateDefaultSocketServer())
	, b_running_(false)
{
	aio_socket_init(1);
}
RtmpIO::~RtmpIO(void)
{
	if (b_running_) {
		b_running_ = false;
		rtc::Thread::Stop();
	}

	aio_socket_clean();
}

void RtmpIO::Attach(RtmpIOTick*ptr)
{
	bool needStart = false;
	{
		rtc::CritScope l(&cs_rtmp_);
		if (map_rtmp_.size() == 0) {
			needStart = true;

		}
		map_rtmp_[ptr] = ptr;
	}

	if (needStart) {
		b_running_ = true;
		rtc::Thread::Start();
	}
}
void RtmpIO::Detach(RtmpIOTick*ptr)
{
	bool needStop = false;
	{
		rtc::CritScope l(&cs_rtmp_);
		if (map_rtmp_.find(ptr) != map_rtmp_.end()) {
			ptr->OnRtmpIODetach();
			map_rtmp_.erase(ptr);

			if (map_rtmp_.size() == 0) {
				needStop = true;
			}
		}
	}

	if (needStop) {
		b_running_ = false;
		rtc::Thread::Stop();
	}
}

void RtmpIO::Run()
{
	

	while (b_running_)
	{
		if (aio_socket_process(1) > 0)
		{
			aio_timeout_process();
		}

		{
			rtc::CritScope l(&cs_rtmp_);
			MapRtmp::iterator itrr = map_rtmp_.begin();
			while (itrr != map_rtmp_.end()) {
				itrr->second->OnRtmpIOTick();
				itrr++;
			}
		}

		rtc::Thread::SleepMs(1);
	}

	
}
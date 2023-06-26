#ifndef __MGR_RENDER_H__
#define __MGR_RENDER_H__
#include <map>
#include "../webrtc/rtc_base/deprecated/recursive_critical_section.h"
#include "../VideoRender/video_renderer.h"

class MgrRender
{
public:
	MgrRender(void);
	virtual ~MgrRender(void);

	void SetRender(const char*pId, webrtc::VideoRenderer*render);

	void SetRotation(const char* pId, int nRotation);

	void SetFillMode(const char* pId, int nMode);

	void DoRenderFrame(const char*pId, const webrtc::VideoFrame& frame);

private:
	typedef std::map<std::string, webrtc::VideoRenderer*>MapRender;
	rtc::RecursiveCriticalSection cs_renders_;
	MapRender	map_renders_;

};

#endif // __MGR_RENDER_H__
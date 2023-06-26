#ifndef __AR_WRITER_H__
#define __AR_WRITER_H__
#include "ARDef.h"
#include "ARCodec.h"

typedef enum ArWriterState
{
	WS_Init = 0,			// ��ʼ��״̬
	WS_Connecting = 1,		// ����������
	WS_Connected = 2,		// ���ӳɹ�
	WS_Failed = 3,			// ʧ��
	WS_Ended = 4,			// ����״̬
}ArWriterState;

class ArWriterEvent
{
public:
	ArWriterEvent(void) {};
	virtual ~ArWriterEvent(void) {};

	virtual void OnArWriterStateChange(ArWriterState oldState, ArWriterState newState) = 0;
	virtual void OnArWriterStats(const char* strJSonDetail) = 0;
};

class ArWriter
{
public:
	virtual ~ArWriter(void) {};

	void SetCallback(ArWriterEvent* callback) {
		callback_ = callback;
	}

	virtual bool StartTask(const char* strFormat, const char* strUrl) = 0;
	virtual void StopTask() = 0;

	virtual void RunOnce() = 0;

	virtual void SetAudioEnable(bool bEnable) = 0;
	virtual void SetVideoEnable(bool bEnable) = 0;
	virtual void SetAutoRetry(bool bAuto) = 0;

	virtual void SetEncType(CodecType vidType, CodecType audType) = 0;
	virtual int SetAudioEncData(const char* pData, int nLen, int64_t pts, int64_t dts) = 0;
	virtual int SetAudioRawData(char* pData, int len, int sample_hz, int channels, int64_t pts, int64_t dts) = 0;
	virtual int SetVideoEncData(const char* pData, int nLen, bool isKeyframe, int64_t pts, int64_t dts) = 0;

	virtual int RecvBroadcast(const char *url, void (*callback)(uint8_t*, int)) = 0;

protected:
	ArWriter() : callback_(NULL) {};

	ArWriterEvent* callback_;
};

ARP_API ArWriter* ARP_CALL createArWriter();
#endif	// __AR_WRITER_H__
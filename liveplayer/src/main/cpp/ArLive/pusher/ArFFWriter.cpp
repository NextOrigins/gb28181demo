#include "ArWriter.h"
#include "ARFFPlayer.h"
#include <android/log.h>
#include <memory>
#include <string>
#include <vector>
#include "ByteOder.h"
#include "codec/pluginaac.h"
#include "webrtc/rtc_base/time_utils.h"
//#include "../lib/ffmpeg/include/libswresample/swresample.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}

static inline int GetStartPatternSize(const void* data, size_t length, int first_pattern_size)
{
	if (data != nullptr)
	{
		auto buffer = static_cast<const uint8_t*>(data);

		if (first_pattern_size > 0)
		{
			if ((buffer[0] == 0x00) && (buffer[1] == 0x00))
			{
				if (
					(first_pattern_size == 4) &&
					((length >= 4) && (buffer[2] == 0x00) && (buffer[3] == 0x01)))
				{
					// 0x00 0x00 0x00 0x01 pattern
					return 4;
				}
				else if (
					(first_pattern_size == 3) &&
					((length >= 3) && (buffer[2] == 0x01)))
				{
					// 0x00 0x00 0x01 pattern
					return 3;
				}
			}

			// pattern_size must be the same with the first_pattern_size
			return -1;
		}
		else
		{
			// probe mode

			if ((length >= 4) && ((buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0x00) && (buffer[3] == 0x01)))
			{
				// 0x00 0x00 0x00 0x01 pattern
				return 4;
			}
			if ((length >= 3) && ((buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0x01)))
			{
				// 0x00 0x00 0x01 pattern
				return 3;
			}
		}
	}

	return -1;
}

int ConvertAnnexbToAvcc(const char* pData, int nLen, char* outData, int outSize)
{
	// size_t total_pattern_length = 0;

	auto buffer = pData;
	size_t remained = nLen;
	off_t offset = 0;
	off_t last_offset = 0;

	char* avcc_data = outData;
	char* byte_stream = avcc_data;
	int avcc_len = 0;

	// This code assumes that (NALULengthSizeMinusOne == 3)
	while (remained > 0)
	{
		if (*buffer == 0x00)
		{
			auto pattern_size = GetStartPatternSize(buffer, remained, 0);

			if (pattern_size > 0)
			{
				if (last_offset < offset)
				{
					SetBE32(byte_stream, offset - last_offset);
					byte_stream += sizeof(int32_t);
					memcpy(byte_stream, pData + last_offset, offset - last_offset);
					byte_stream += offset - last_offset;

					avcc_len += (sizeof(int32_t) + (offset - last_offset));
					last_offset = offset;
				}

				buffer += pattern_size;
				offset += pattern_size;
				last_offset += pattern_size;
				remained -= pattern_size;

				continue;
			}
		}

		buffer++;
		offset++;
		remained--;
	}

	if (last_offset < offset)
	{
		// Append remained data
		SetBE32(byte_stream, offset - last_offset);
		byte_stream += sizeof(int32_t);
		memcpy(byte_stream, pData + last_offset, offset - last_offset);
		byte_stream += offset - last_offset;

		avcc_len += (sizeof(int32_t) + (offset - last_offset));
		last_offset = offset;
	}

	return avcc_len;
}
#if 0
static struct AVFrameDeleter {
	void operator()(AVFrame* ptr) const { av_frame_free(&ptr); }
};
#endif
class ArFFWriter : public ArWriter
{
public:
	ArFFWriter(void);
	virtual ~ArFFWriter(void);



	//* For ArWriter
	virtual bool StartTask(const char* strFormat, const char* strUrl);
	virtual void StopTask();

	virtual void RunOnce();

	virtual void SetAudioEnable(bool bEnable);
	virtual void SetVideoEnable(bool bEnable);
	virtual void SetAutoRetry(bool bAuto);

	virtual void SetEncType(CodecType vidType, CodecType audType);
	virtual int SetAudioEncData(const char* pData, int nLen, int64_t pts, int64_t dts);
	virtual int SetAudioRawData(char* pData, int len, int sample_hz, int channels, int64_t pts, int64_t dts);
	virtual int SetVideoEncData(const char* pData, int nLen, bool isKeyframe, int64_t pts, int64_t dts);

	virtual int RecvBroadcast(const char *url, void (*callback)(uint8_t*, int));

protected:
	bool Connect();
	void Release();
private:
	CodecType vid_codec_type_;
	CodecType aud_codec_type_;
	bool b_vid_enable_;
	bool b_aud_enable_;

	char* avcc_data_;
	int avcc_size_;

private:
	AVFormatContext* format_context_;

	AVStream* vid_stream_;
	AVStream* aud_stream_;

	aac_enc_t aac_enc_;

private:
	ArWriterState now_state_;
	bool b_auto_retry_;
	uint32_t n_next_retry_time_;

	std::string str_format_;
	std::string str_url_;
};

ArWriter* createArWriter()
{
	return new ArFFWriter();
}

ArFFWriter::ArFFWriter(void)
	: vid_codec_type_(CT_H264)
	, aud_codec_type_(CT_AAC)
	, b_vid_enable_(true)
	, b_aud_enable_(true)
	, avcc_data_(NULL)
	, avcc_size_(0)
	, format_context_(NULL)
	, vid_stream_(NULL)
	, aud_stream_(NULL)
	, aac_enc_(NULL)
	, now_state_(WS_Init)
	, b_auto_retry_(false)
	, n_next_retry_time_(0)
{
	aac_enc_ = aac_encoder_open(2, 44100, 16, 64000, true);
}
ArFFWriter::~ArFFWriter(void)
{
	if (aac_enc_ != NULL) {
		aac_encoder_close(aac_enc_);
		aac_enc_ = NULL;
	}
}



//* For ArWriter
bool ArFFWriter::StartTask(const char* strFormat, const char* strUrl)
{
	str_format_ = strFormat;
	str_url_ = strUrl;
	n_next_retry_time_ = rtc::Time32();
	return true;
}
void ArFFWriter::StopTask()
{
	Release();
}

void ArFFWriter::RunOnce()
{
	if (n_next_retry_time_ != 0 && n_next_retry_time_ <= rtc::Time32()) {
		n_next_retry_time_ = 0;
		ArWriterState oldState = now_state_;
		now_state_ = WS_Connecting;
		if (callback_ != NULL) {
			callback_->OnArWriterStateChange(oldState, now_state_);
		}
		bool bRet = Connect();

		oldState = now_state_;

		if (!bRet) {
			now_state_ = WS_Failed;
			if (callback_ != NULL) {
				callback_->OnArWriterStateChange(oldState, now_state_);
			}
			Release();

			if (b_auto_retry_) {
				n_next_retry_time_ = rtc::Time32() + 1500;
			}
			else {
				oldState = now_state_;
				now_state_ = WS_Ended;
				if (callback_ != NULL) {
					callback_->OnArWriterStateChange(oldState, now_state_);
				}
			}
		}
		else {
			now_state_ = WS_Connected;
			if (callback_ != NULL) {
				callback_->OnArWriterStateChange(oldState, now_state_);
			}
		}
	}
}

void ArFFWriter::SetAudioEnable(bool bEnable)
{
	b_aud_enable_ = bEnable;
}
void ArFFWriter::SetVideoEnable(bool bEnable)
{
	b_vid_enable_ = bEnable;
}
void ArFFWriter::SetAutoRetry(bool bAuto)
{
	b_auto_retry_ = bAuto;
}

void ArFFWriter::SetEncType(CodecType vidType, CodecType audType)
{
	vid_codec_type_ = vidType;
	aud_codec_type_ = audType;
}

int ArFFWriter::SetAudioEncData(const char* pData, int nLen, int64_t pts, int64_t dts)
{
	if (aud_stream_ == NULL) {
		return -1;
	}
	if (!b_aud_enable_) {
		return -2;
	}
	if (now_state_ != WS_Connected) {
		return -3;
	}
	// Make avpacket
	AVPacket av_packet;
	av_init_packet(&av_packet);	//@Eric - 20230323 - ����ʹ��av_packet = { 0 };��ʼ��������ʹ��av_init_packet��ʼ�� - ����¼�Ƶ��ļ������Բ�
	av_packet.stream_index = aud_stream_->index;
	av_packet.flags = 0;// (flag == MediaPacketFlag::Key) ? AV_PKT_FLAG_KEY : 0;
	//av_packet.pts = av_packet.dts = AV_NOPTS_VALUE;
	//av_packet.pts = av_rescale_q(pts, AVRational{ 1, 1000 }, aud_stream_->time_base); //pts;// 
	//av_packet.dts = av_rescale_q(dts, AVRational{ 1, 1000 }, aud_stream_->time_base);//dts;// 
	//printf("aud pts: %lld dts: %lld\r\n", pts, dts);

	av_packet.pts = pts;
	av_packet_rescale_ts(&av_packet, AVRational{ 1, 1000 }, aud_stream_->time_base);
	//av_packet.duration = 0;

	if (strcmp(format_context_->oformat->name, "flv") == 0)
	{
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
		av_packet.pts = pts;
		av_packet.dts = dts;
	}
	else if (strcmp(format_context_->oformat->name, "mp4") == 0)
	{
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}
	else if (strcmp(format_context_->oformat->name, "mpegts") == 0 || strcmp(format_context_->oformat->name, "rtp_mpegts") == 0)
	{
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}
	else {
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}

	int error = av_interleaved_write_frame(format_context_, &av_packet);
	if (error != 0)
	{
		char errbuf[256];
		av_strerror(error, errbuf, sizeof(errbuf));
		printf("Send audio packet error(%d:%s) \r\n", error, errbuf);

		ArWriterState oldState = now_state_;
		now_state_ = WS_Failed;
		if (callback_ != NULL) {
			callback_->OnArWriterStateChange(oldState, now_state_);
		}
		if (b_auto_retry_) {
			n_next_retry_time_ = rtc::Time32() + 1500;
		}
		else {
			oldState = now_state_;
			now_state_ = WS_Ended;
			if (callback_ != NULL) {
				callback_->OnArWriterStateChange(oldState, now_state_);
			}
		}
		Release();
		return false;
	}
	return 0;
}
int ArFFWriter::SetAudioRawData(char* pData, int len, int sample_hz, int channels, int64_t pts, int64_t dts)
{
	const int kMaxDataSizeSamples = 4196;
	if (44100 != sample_hz || 2 != channels) {
		return -1;
	}
	else {
		int status = 0;
		if (aac_enc_)
		{
			unsigned int outlen = 0;
			uint32_t curtime = rtc::Time32();
			uint8_t encoded[1024];
			status = aac_encoder_encode_frame(aac_enc_, (uint8_t*)pData, len, encoded, &outlen);
			if (outlen > 0)
			{
				SetAudioEncData((char*)encoded, outlen, pts, dts);
			}
		}
	}
	return 0;
}
int ArFFWriter::SetVideoEncData(const char* pData, int nLen, bool isKeyframe, int64_t pts, int64_t dts)
{
	if (vid_stream_ == NULL) {
		return -1;
	}
	if (!b_vid_enable_) {
		return -2;
	}
	if (now_state_ != WS_Connected) {
		return -3;
	}
	// Make avpacket
	AVPacket av_packet;
	av_init_packet(&av_packet);	//@Eric - 20230323 - ����ʹ��av_packet = { 0 };��ʼ��������ʹ��av_init_packet��ʼ��
	av_packet.stream_index = vid_stream_->index;
	av_packet.flags = isKeyframe ? AV_PKT_FLAG_KEY : 0;
	//av_packet.pts = av_packet.dts = AV_NOPTS_VALUE;
	//av_packet.pts = av_rescale_q(pts, AVRational{ 1, 1000 }, vid_stream_->time_base);	//pts;// 
	//av_packet.dts = av_rescale_q(dts, AVRational{ 1, 1000 }, vid_stream_->time_base);	//dts;// 
	//printf("vid pts: %lld dts: %lld\r\n", pts, dts);

	av_packet.pts = pts;
	av_packet_rescale_ts(&av_packet, AVRational{ 1, 1000 }, vid_stream_->time_base);
	//av_packet.duration = 0;
	
	if (strcmp(format_context_->oformat->name, "flv") == 0)
	{
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
		//* �����FLV��ʱ�����ԭʼ�ľ�����
		av_packet.pts = pts;	
		av_packet.dts = dts;
	}
	else if (strcmp(format_context_->oformat->name, "mp4") == 0)
	{
		if (avcc_size_ <= nLen + 32) {
			if (avcc_data_ != NULL) {
				delete[] avcc_data_;
				avcc_data_ = NULL;
			}
			avcc_size_ = 0;
		}
		if (avcc_data_ == NULL) {
			avcc_size_ = nLen + 32;
			avcc_data_ = new char[avcc_size_];
		}
#if 0
		int avccLen = ConvertAnnexbToAvcc(pData, nLen, avcc_data_, avcc_size_);
		av_packet.size = avccLen;
		av_packet.data = (uint8_t*)avcc_data_;
#endif
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}
	else if (strcmp(format_context_->oformat->name, "mpegts") == 0 || strcmp(format_context_->oformat->name, "rtp_mpegts") == 0)
	{
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}
	else {
		av_packet.size = nLen;
		av_packet.data = (uint8_t*)pData;
	}

	int error = av_interleaved_write_frame(format_context_, &av_packet);
	if (error != 0)
	{
		char errbuf[256];
		av_strerror(error, errbuf, sizeof(errbuf));

		printf("Send video packet error(%d:%s) \r\n", error, errbuf);
		ArWriterState oldState = now_state_;
		now_state_ = WS_Failed;
		if (callback_ != NULL) {
			callback_->OnArWriterStateChange(oldState, now_state_);
		}
		if (b_auto_retry_) {
			n_next_retry_time_ = rtc::Time32() + 1500;
		}
		else {
			oldState = now_state_;
			now_state_ = WS_Ended;
			if (callback_ != NULL) {
				callback_->OnArWriterStateChange(oldState, now_state_);
			}
		}
		Release();
		return false;
	}
	return 0;
}


bool ArFFWriter::Connect()
{
	{//1, 
		//const AVOutputFormat* output_format = nullptr;
		AVOutputFormat* output_format = nullptr;
		output_format = av_guess_format(str_format_.c_str(), nullptr, nullptr);
		if (output_format == nullptr)
		{
			Release();
			return false;
		}
		int error = avformat_alloc_output_context2(&format_context_, output_format, nullptr, str_url_.c_str());
		if (error < 0)
		{
			char errbuf[256];
			av_strerror(error, errbuf, sizeof(errbuf));

			printf("Could not create output context. error(%d:%s), path(%s) \r\n", error, errbuf, str_url_.c_str());
			Release();
			return false;
		}
	}

	{//2, 
		if (vid_codec_type_ != CT_None)
		{//* Video
			AVCodecID codecId = (vid_codec_type_ == CT_H264) ? AV_CODEC_ID_H264 : (vid_codec_type_ == CT_H265) ? AV_CODEC_ID_H265
				: AV_CODEC_ID_NONE;
			const AVCodec* codec = avcodec_find_encoder(codecId);
			vid_stream_ = avformat_new_stream(format_context_, codec);
			AVCodecParameters* codecpar = vid_stream_->codecpar;
			int w = 1920;
			int h = 1080;
			int nFps = 30;
			int nBitrate = 1024;

			codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			codecpar->codec_id = codecId;

			codecpar->codec_tag = 0;
			codecpar->bit_rate = nBitrate * 1000;
			codecpar->width = w;
			codecpar->height = h;
			codecpar->format = AV_PIX_FMT_YUV420P;
			codecpar->sample_aspect_ratio = AVRational{ 1, 1 };

#if 0
			if (vid_codec_type_ == CT_H264)
			{
				//* �����ɹ�������Bվ���Ų��ˣ� https://www.cnblogs.com/subo_peng/p/7800658.html
				unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
					  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
				codecpar->extradata_size = 23;
				codecpar->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
				if (codecpar->extradata == NULL) {
					printf("could not av_malloc the video params extradata! \r\n");
					return -1;
				}
				memcpy(codecpar->extradata, sps_pps, 23);
			}
			else if (vid_codec_type_ == CT_H265) {
			}
#else
			AVCodecContext* vidEncodeCtx = NULL;
			vidEncodeCtx = avcodec_alloc_context3(codec);
			if (vidEncodeCtx != NULL) {
				vidEncodeCtx->codec_type = AVMEDIA_TYPE_VIDEO;
				vidEncodeCtx->codec_id = codecId;
				vidEncodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;
				vidEncodeCtx->extradata = nullptr;
				vidEncodeCtx->extradata_size = 0;

				// If this is ever increased, look at |vidEncodeCtx->thread_safe_callbacks| and
				// make it possible to disable the thread checker in the frame buffer pool.
				vidEncodeCtx->thread_count = 1;
				vidEncodeCtx->thread_type = FF_THREAD_SLICE;

				// Function used by FFmpeg to get buffers to store decoded frames in.
				//vidEncodeCtx->get_buffer2 = AVGetBuffer2;
				// |get_buffer2| is called with the context, there |opaque| can be used to get
				// a pointer |this|.
				//vidEncodeCtx->opaque = this;
				{

					vidEncodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;    //�����ԭʼ���ݸ�ʽ
					vidEncodeCtx->codec_type = AVMEDIA_TYPE_VIDEO; //ָ��Ϊ��Ƶ����
					vidEncodeCtx->width = w;      //�ֱ��ʿ�
					vidEncodeCtx->height = h;     //�ֱ��ʸ�
					vidEncodeCtx->channels = 0;           //��Ƶͨ����
					vidEncodeCtx->time_base = { 1, nFps };  //ʱ�������ʾÿ��ʱ��̶��Ƕ����롣
					vidEncodeCtx->framerate = { nFps, 1 };  //֡�ʣ�û��
					vidEncodeCtx->gop_size = 3 * nFps;    //ͼ���������ؼ�֡��I֡���ľ��룬Ҳ����һ��֡������ ԭ����10
					vidEncodeCtx->max_b_frames = 0;            //ָ��B֡������B֡��˫��ο�֡��������B֡��ѹ���ʸ��ߵ����ӳ�Ҳ��
					// some formats want stream headers to be separate
					if (vidEncodeCtx->flags & AVFMT_GLOBALHEADER)
					{
						vidEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
					}

					// ��Ƶ���������õ����ʿ��Ʒ�ʽ����abr(ƽ������)��crf(�㶨����)��cqp(�㶨����)��
					// ffmpeg��AVCodecContext��ʾ�ṩ�����ʴ�С�Ŀ��Ʋ��������ǲ�û���ṩ�����Ŀ��Ʒ�ʽ��
					// ffmpeg�����ʿ��Ʒ�ʽ��Ϊ���¼��������
					// 1.���������AVCodecContext��bit_rate�Ĵ�С�������abr�Ŀ��Ʒ�ʽ��
					// 2.���û������AVCodecContext�е�bit_rate����Ĭ�ϰ���crf��ʽ���룬crfĬ�ϴ�СΪ23����ֵ������qpֵ��ͬ����ʾ��Ƶ��������
					// 3.����û����Լ����ã�����Ҫ����av_opt_set��������AVCodecContext��priv_data����������������ֿ��Ʒ�ʽ��ʵ�ִ��룺
					vidEncodeCtx->bit_rate = nBitrate * 1000; //���ʡ��������ù̶�����Ӧ��ûɶ�á�
					vidEncodeCtx->rc_max_rate = vidEncodeCtx->bit_rate + 10000;
					vidEncodeCtx->rc_min_rate = vidEncodeCtx->bit_rate - 10000;
					///�㶨����
					// ���������ķ�ΧΪ0~51������0Ϊ����ģʽ��23Ϊȱʡֵ��51���������ġ�������ԽС��ͼ������Խ�á��������Ͻ���18~28��һ������ķ�Χ��18��������Ϊ���Ӿ��Ͽ�������ģ����������Ƶ������������Ƶһģһ������˵����޼������Ӽ����ĽǶ�����������Ȼ������ѹ����
					// ��Crfֵ��6��������ʴ�ż���һ�룻��Crfֵ��6��������ʷ�����ͨ�����ڱ�֤�ɽ�����Ƶ������ǰ����ѡ��һ������Crfֵ����������Ƶ�����ܺã��Ǿͳ���һ�������ֵ��������������㣬�Ǿͳ���һ��Сһ��ֵ��
					//av_opt_set(vidEncodeCtx->priv_data, "crf", "21.000", AV_OPT_SEARCH_CHILDREN);

					//av_opt_set(vidEncodeCtx->priv_data, "preset", "slow", 0);       //����ѹ�����룬���Ŀ��Ա�֤��Ƶ����
					//av_opt_set(vidEncodeCtx->priv_data, "preset", "veryfast", 0);
					//av_opt_set(vidEncodeCtx->priv_data, "preset", "ultrafast", 0);    //���ٱ��룬������ʧ����
					//av_opt_set(vidEncodeCtx->priv_data, "tune", "zerolatency", 0);  //�����ڿ��ٱ���͵��ӳ���ʽ����,���ǻ��������
				}

				//������Ԥ��
				AVDictionary* param = 0;
				if (vidEncodeCtx->codec_id == AV_CODEC_ID_H264)
				{
					av_dict_set(&param, "preset", "ultrafast", 0);
					//av_dict_set(&param, "preset", "superfast", 0);
					av_dict_set(&param, "tune", "zerolatency", 0); //ʵ��ʵʱ����
					av_dict_set(&param, "profile", "baseline", 0);
				}
				else if (vidEncodeCtx->codec_id == AV_CODEC_ID_H265)
				{
					av_dict_set(&param, "preset", "ultrafast", 0);
					av_dict_set(&param, "tune", "zerolatency", 0);
					av_dict_set(&param, "profile", "main", 0);	// main
				}
				vidEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	//@Eric - ���Ҫ��open�������Ժ�AVCodecContext extradata���� SPS,PPS ��Ϣ��Ҫ����
				int res = avcodec_open2(vidEncodeCtx, codec, &param);
				if (res < 0) {
					char buff[128] = { 0 };
					//av_strerror(res, buff, 128);
					if (vid_codec_type_ == CT_H264)
					{
						//* �����ɹ�������Bվ���Ų��ˣ� https://www.cnblogs.com/subo_peng/p/7800658.html
						unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
							  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
						codecpar->extradata_size = 23;
						codecpar->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
						if (codecpar->extradata == NULL) {
							printf("could not av_malloc the video params extradata! \r\n");
							return -1;
						}
						memcpy(codecpar->extradata, sps_pps, 23);
					}
					else if (vid_codec_type_ == CT_H265) {
					}
				}
				else {
					if (vidEncodeCtx->extradata_size > 0 && vidEncodeCtx->extradata != NULL) {
						codecpar->extradata_size = vidEncodeCtx->extradata_size;
						codecpar->extradata = (uint8_t*)av_malloc(vidEncodeCtx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
						if (codecpar->extradata == NULL) {
							printf("could not av_malloc the video params extradata! \r\n");
							return -1;
						}
						memcpy(codecpar->extradata, vidEncodeCtx->extradata, vidEncodeCtx->extradata_size);
					}
				}

				avcodec_free_context(&vidEncodeCtx);
				vidEncodeCtx = NULL;
			}
#endif
			vid_stream_->time_base = AVRational{ 1, 90000 };
		}

		if (aud_codec_type_ != CT_None)
		{//* Audio
			AVCodecID codecId = (aud_codec_type_ == CT_AAC) ? AV_CODEC_ID_AAC :
				(aud_codec_type_ == CT_MP3) ? AV_CODEC_ID_MP3 :
				(aud_codec_type_ == CT_OPUS) ? AV_CODEC_ID_OPUS : (aud_codec_type_ == CT_G711A) ? AV_CODEC_ID_PCM_ALAW : AV_CODEC_ID_NONE;
			const AVCodec* codec = avcodec_find_encoder(codecId);
			aud_stream_ = avformat_new_stream(format_context_, codec);
			AVCodecParameters* codecpar = aud_stream_->codecpar;

			codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
			codecpar->codec_id = codecId;
			codecpar->bit_rate = 64000;
			if (codecId == AV_CODEC_ID_PCM_ALAW) {
				codecpar->channels = static_cast<int>(1);
				codecpar->channel_layout = AV_CH_LAYOUT_MONO;
				codecpar->sample_rate = 8000;
				codecpar->frame_size = 4096;  // TODO: Need to Frame Size
			}
			else {
				codecpar->channels = static_cast<int>(2);
				codecpar->channel_layout = AV_CH_LAYOUT_STEREO;//AV_CH_LAYOUT_MONO
				codecpar->sample_rate = 44100;
				codecpar->frame_size = 1024;  // TODO: Need to Frame Size
			}
			codecpar->codec_tag = 0;

			// set extradata for aac_specific_config
#if 0
			if (track_info->GetExtradata() != nullptr)
			{
				codecpar->extradata_size = track_info->GetExtradata()->GetLength();
				codecpar->extradata = (uint8_t*)av_malloc(codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
				memset(codecpar->extradata, 0, codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
				memcpy(codecpar->extradata, track_info->GetExtradata()->GetDataAs<uint8_t>(), codecpar->extradata_size);
			}
#else
			AVCodecContext* audEncodeCtx = NULL;
			audEncodeCtx = avcodec_alloc_context3(codec);
			if (audEncodeCtx != NULL) {
				audEncodeCtx->sample_fmt = codec->sample_fmts[0];	// ����һ�£�����avcodec_open2�᷵��-22��Invalid agument
				audEncodeCtx->bit_rate = 64000;
				if (codecId == AV_CODEC_ID_PCM_ALAW) {
					audEncodeCtx->time_base = AVRational{ 1, 48000 };
					audEncodeCtx->channels = static_cast<int>(1);
					audEncodeCtx->channel_layout = AV_CH_LAYOUT_MONO;
					codecpar->sample_rate = 8000;
					codecpar->frame_size = 4096;  // TODO: Need to Frame Size
				}
				else {
					audEncodeCtx->channels = static_cast<int>(2);
					audEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;//AV_CH_LAYOUT_MONO
					audEncodeCtx->sample_rate = 44100;
					audEncodeCtx->frame_size = 1024;  // TODO: Need to Frame Size
				}
				AVDictionary* param = 0;
				audEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	//@Eric - ���Ҫ��open�������Ժ�AVCodecContext extradata���� SPS,PPS ��Ϣ��Ҫ����
				int res = avcodec_open2(audEncodeCtx, codec, NULL);
				if (res < 0) {
					char buff[128] = { 0 };
					av_strerror(res, buff, 128);
					printf("%s\r\n", buff);
				}
				else {
					if (audEncodeCtx->extradata_size > 0 && audEncodeCtx->extradata != NULL) {
						codecpar->extradata_size = audEncodeCtx->extradata_size;
						codecpar->extradata = (uint8_t*)av_malloc(audEncodeCtx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
						if (codecpar->extradata == NULL) {
							printf("could not av_malloc the video params extradata! \r\n");
							return -1;
						}
						memcpy(codecpar->extradata, audEncodeCtx->extradata, audEncodeCtx->extradata_size);
					}
				}

				avcodec_free_context(&audEncodeCtx);
				audEncodeCtx = NULL;
			}
#endif
			aud_stream_->time_base = AVRational{ 1, 48000 };
		}
	}

	{//3, 
		AVDictionary* options = nullptr;
		// Compatibility with specific RTMP servers
		// tc_url : rtmp://[host]:[port]/[app_name]
		if (str_url_.find("rtmp://") != std::string::npos || str_url_.find("rtmps://") != std::string::npos) {
			char tc_url[1024];
			sprintf(tc_url, "%.*s", strrchr(str_url_.c_str(), '/') - str_url_.c_str(), str_url_.c_str());
			av_dict_set(&options, "rtmp_tcurl", tc_url, 0);
			av_dict_set(&options, "fflags", "flush_packets", 0);
			av_dict_set(&options, "rtmp_flashver", "FMLE/3.0 (compatible; FMSc/1.0)", 0);
		}
		else if (str_url_.find("rtsp://") != std::string::npos || str_url_.find("rtp://") != std::string::npos){
			av_dict_set(&options, "buffer_size", "4096000", 0); //���û����С
			av_dict_set(&options, "rtsp_transport", "tcp", 0);  //��tcp�ķ�ʽ��,
			av_dict_set(&options, "stimeout", "5000000", 0);    //���ó�ʱ�Ͽ�����ʱ�䣬��λus,   5s
			av_dict_set(&options, "max_delay", "500000", 0);    //�������ʱ��
		}
		else {
			av_dict_set(&options, "fflags", "flush_packets", 0);
		}

		if (!(format_context_->oformat->flags & AVFMT_NOFILE))
		{
			int error = avio_open2(&format_context_->pb, format_context_->url, AVIO_FLAG_WRITE, nullptr, nullptr);
			if (error < 0)
			{
				char errbuf[256];
				av_strerror(error, errbuf, sizeof(errbuf));

				printf("Error opening file. error(%d:%s), url(%s) \r\n", error, errbuf, format_context_->url);
				Release();
				return false;
			}
		}
		int error = avformat_write_header(format_context_, &options);
		if (error < 0)
		{
			char errbuf[256];
			av_strerror(error, errbuf, sizeof(errbuf));
			printf("Could not create header \r\n");
			Release();
			return false;
		}

		av_dump_format(format_context_, 0, format_context_->url, 1);
#if 0
		if (format_context_->oformat != nullptr)
		{
			[[maybe_unused]] auto oformat = format_context_->oformat;
			printf("name : %s \r\n", oformat->name);
			printf("long_name : %s \r\n", oformat->long_name);
			printf("mime_type : %s \r\n", oformat->mime_type);
			printf("audio_codec : %d \r\n", oformat->audio_codec);
			printf("video_codec : %d \r\n", oformat->video_codec);
		}
#endif
	}

	return true;
}
void ArFFWriter::Release()
{
	if (format_context_ != nullptr)
	{
		if (format_context_->pb != nullptr)
		{
			av_write_trailer(format_context_);
			avformat_close_input(&format_context_);
		}

		avformat_free_context(format_context_);

		format_context_ = nullptr;
	}

	vid_stream_ = NULL;
	aud_stream_ = NULL;

	if (avcc_data_ != NULL) {
		delete avcc_data_;
		avcc_data_ = NULL;
	}
}

int ArFFWriter::RecvBroadcast(const char *url, void (*callback)(uint8_t*, int))
{
	//av_register_all();
    avformat_network_init();
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "timeout", "6000", 0);

	AVFormatContext *format_ctx = NULL;
	__android_log_print(ANDROID_LOG_ERROR, "JNI", "Before open, Url=%s", url);
    if (avformat_open_input(&format_ctx, url, nullptr, &opts) < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: can not open video file");
        return 1;
    }

	int result = avformat_find_stream_info(format_ctx, nullptr);
	if (result < 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: can not find video file stream info");
		return 1;
	}

	int audio_stream_index = -1;
	for (int i = 0; i < format_ctx->nb_streams; i++)
	{
		if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			audio_stream_index = i;
	}
	if (audio_stream_index == -1)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: can not find audio stream");
		return 1;
	}

    AVCodecParameters *audio_codecpar = format_ctx->streams[audio_stream_index]->codecpar;
    AVCodec *audio_codec = avcodec_find_decoder(audio_codecpar->codec_id);
    if (audio_codec == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: cannot find audio codec.");
        return 1;
    }

    AVCodecContext *audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (audio_codec_ctx == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: alloc audio codec failed.");
        return 1;
    }

    if (avcodec_parameters_to_context(audio_codec_ctx, audio_codecpar) < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: duplicated failed.");
        return 1;
    }

    if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: open failed.");
        return 1;
    }

    SwrContext *swr_ctx = swr_alloc();
	if (swr_ctx == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: swr alloc failed.");
		return 1;
	}

	av_opt_set_int(swr_ctx, "in_channel_layout", audio_codec_ctx->channel_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", audio_codec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_codec_ctx->sample_fmt, 0);
	av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

	if (swr_init(swr_ctx) < 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: Swr init failed.");
		return 1;
	}

	AVFrame *audio_frame = av_frame_alloc();
	if (audio_frame == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_frame_alloc failed.");
		return 1;
	}

	AVPacket *audio_packet = av_packet_alloc();
	if (audio_packet == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_packet_alloc failed.");
		return 1;
	}

	uint8_t *buffer = nullptr;
	int buffer_size = 0;
	while (true)
	{
		if (av_read_frame(format_ctx, audio_packet) < 0)
		{
			__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed.");
			break;
		}
		// 判断音频包是否属于音频流
		if (audio_packet->stream_index == audio_stream_index) {
			// 将音频包发送给音频解码器
			if (avcodec_send_packet(audio_codec_ctx, audio_packet) < 0) {
				// 处理发送失败的情况
				break;
			}

			// 从音频解码器接收一个音频帧
			if (avcodec_receive_frame(audio_codec_ctx, audio_frame) < 0) {
				// 处理接收失败的情况
                __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed1.");
                break;
            }
            // 判断音频包是否属于音频流
            if (audio_packet->stream_index == audio_stream_index) {
                // 将音频包发送给音频解码器
                if (avcodec_send_packet(audio_codec_ctx, audio_packet) < 0) {
                    // 处理发送失败的情况
                    __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed2.");
                    break;
                }
                // 判断音频包是否属于音频流
                if (audio_packet->stream_index == audio_stream_index) {
                    // 将音频包发送给音频解码器
                    if (avcodec_send_packet(audio_codec_ctx, audio_packet) < 0) {
                        // 处理发送失败的情况
                        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed3.");
                        break;
                    }
                    break;
                }
				break;
			}

			// 计算重采样后的缓冲区大小，并分配内存
			int out_samples = av_rescale_rnd(
					swr_get_delay(swr_ctx, audio_codec_ctx->sample_rate) + audio_frame->nb_samples,
					44100,
					audio_codec_ctx->sample_rate,
					AV_ROUND_UP);
			if (out_samples <= 0)
			{
				__android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: out sample less than 1.");
				break;
			}
			int out_buffer_size = av_samples_get_buffer_size(
					NULL,
					2,
					out_samples,
					AV_SAMPLE_FMT_S16,
					0
					);
			if (out_buffer_size > buffer_size) {
				buffer_size = out_buffer_size;
				buffer = (uint8_t *)realloc(buffer, buffer_size);
				if (buffer == NULL) {
					// 处理内存分配失败的情况
                    __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed4.");
					break;
				}
			}

			// 将音频帧数据重采样，并存储到缓冲区中
			int out_count = swr_convert(swr_ctx,
										&buffer,
										out_samples,
										(const uint8_t **)audio_frame->data,
										audio_frame->nb_samples);
			if (out_count < 0) {
				// 处理重采样失败的情况
                __android_log_print(ANDROID_LOG_ERROR, "JNI", "Player Error: av_read_frame failed5.");
				break;
			}

			// 将缓冲区中的数据写入到Android AudioTrack中，进行播放
            __android_log_print(ANDROID_LOG_ERROR, "JNI", "=-=-=-=-=-=-=-=-=-=-=-=-=-=- Player, callback;");
			if (callback)
				callback(buffer, out_buffer_size);
			//write_to_audio_track(buffer, out_buffer_size);
		}

// 释放音频包内存
		av_packet_unref(audio_packet);
	}

	av_packet_free(&audio_packet);
	av_frame_free(&audio_frame);
	free(buffer);
	swr_free(&swr_ctx);
	avcodec_close(audio_codec_ctx);
	avcodec_free_context(&audio_codec_ctx);
	avformat_close_input(&format_ctx);
	avformat_network_deinit();

	return 0;
}

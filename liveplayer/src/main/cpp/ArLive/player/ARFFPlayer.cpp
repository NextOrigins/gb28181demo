#include "ARFFPlayer.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#ifdef WEBRTC_80
#include "rtc_base/internal/default_socket_server.h"
#endif
#include "common_audio/signal_processing/include/signal_processing_library.h"

static uint8_t startcode[4] = { 00,00,00,01 };
#define SAMPLE_SIZE			262144
static const size_t kMaxDataSizeSamples = 3840;
static const int64_t kBitrateControllerUpdateIntervalMs = 10;
// �ڲ���������
static const AVRational TIMEBASE_MS = { 1, 1000 };
#define fftime_to_milliseconds(ts) (av_rescale(ts, 1000, AV_TIME_BASE))
#define milliseconds_to_fftime(ms) (av_rescale(ms, AV_TIME_BASE, 1000))

static void audio_track_change(uint8_t* pBuffer, int iSize, int AudioChannel, int AudioBits)
{
	int i = 0, w_pos = 0, r_pos = 0, n = 0;

	n = iSize / 4; //ѭ������

	switch (AudioBits)
	{
	case 8:
		//��������ѡ��
		switch (AudioChannel)
		{
		case 1:r_pos = 0;
			w_pos = 1;
			break;             //������

		case 2:r_pos = 1;
			w_pos = 0;
			break;             //������

		default:return;           //������˫����ģʽ��

		}
		//ִ�������л�����  
		for (i = 0; i < n; i++) {
			memcpy(pBuffer + w_pos, pBuffer + r_pos, 1);
			w_pos += 2;
			r_pos += 2;
		}
		break;
	case 16:
		//��������ѡ��
		switch (AudioChannel)
		{
		case 1:r_pos = 0;
			w_pos = 2;
			break;             //������

		case 2:r_pos = 2;
			w_pos = 0;
			break;             //������

		default:return;           //������˫����ģʽ��

		}
		//ִ�������л�����  
		for (i = 0; i < n; i++) {
			memcpy(pBuffer + w_pos, pBuffer + r_pos, 2);
			w_pos += 4;
			r_pos += 4;
		}
		break;
	case 24:
		//��������ѡ��
		switch (AudioChannel)
		{
		case 1:r_pos = 0;
			w_pos = 3;
			break;             //������

		case 2:r_pos = 3;
			w_pos = 0;
			break;             //������

		default:return;           //������˫����ģʽ��

		}
		//ִ�������л�����  
		for (i = 0; i < n; i++) {
			memcpy(pBuffer + w_pos, pBuffer + r_pos, 3);
			w_pos += 6;
			r_pos += 6;
		}
		break;
	case 32:
		//��������ѡ��
		switch (AudioChannel)
		{
		case 1:r_pos = 0;
			w_pos = 4;
			break;             //������

		case 2:r_pos = 4;
			w_pos = 0;
			break;             //������

		default:return;           //������˫����ģʽ��

		}
		//ִ�������л�����  
		for (i = 0; i < n; i++) {
			memcpy(pBuffer + w_pos, pBuffer + r_pos, 4);
			w_pos += 8;
			r_pos += 8;
		}
		break;
	}
	return;
}

int16_t WebRtcSpl_MaxAbsValueW16_I(const int16_t* vector, size_t length) {
	size_t i = 0;
	int absolute = 0, maximum = 0;

	RTC_DCHECK_GT(length, 0);

	for (i = 0; i < length; i++) {
		absolute = abs((int)vector[i]);

		if (absolute > maximum) {
			maximum = absolute;
		}
	}

	// Guard the case for abs(-32768).
	if (maximum > WEBRTC_SPL_WORD16_MAX) {
		maximum = WEBRTC_SPL_WORD16_MAX;
	}

	return (int16_t)maximum;
}

class FFContex
{// �����������ϰ汾FFmpeg�ı���ȫ��ֻҪ��ʼ��һ�μ��ɡ��µ�FFmpeg�汾��������Ҫ�������Ƴ�ʼ������
protected:
	FFContex(void){
		//av_register_all();	// �°汾�Ѿ�����Ҫ
		avformat_network_init();

	};
public:
	static FFContex& Inst() {
		static FFContex gInst;
		return gInst;
	}
	virtual ~FFContex(void) {

		avformat_network_deinit();
	};
};

#if (defined(__APPLE__) && TARGET_OS_MAC || TARGET_OS_IPHONE)
//@derek AVMMediaType replace FFMAVMediaType
static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMMediaType type)
#else
static int open_codec_context(int *stream_idx,
	AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
#endif
{
	int ret, stream_index;
	AVStream *st;
	AVCodec *dec = NULL;
	AVDictionary *opts = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			fprintf(stderr, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set(&opts, "refcounted_frames", "1", 0);
		if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

static int custom_interrupt_callback(void *arg) {
	ARFFPlayer* splayer = (ARFFPlayer*)arg;
	return splayer->Timeout();
}

ARPlayer* ARP_CALL createARPlayer(ARPlayerEvent&callback)
{
	return new ARFFPlayer(callback);
}

ARFFPlayer::ARFFPlayer(ARPlayerEvent&callback)
	: ARPlayer(callback)
#ifdef WEBRTC_80
	, rtc::Thread(rtc::CreateDefaultSocketServer())
#else
	, rtc::Thread(rtc::SocketServer::CreateDefault())
#endif
	, fmt_ctx_(NULL)
	, n_video_stream_idx_(-1)
	, n_audio_stream_idx_(-1)
	, n_seek_to_(0)
	, n_all_duration_(0)
	, n_cur_aud_duration_(0)
	, n_cur_vid_duration_(0)
	, n_total_track_(0)
	, b_running_(false)
	, b_open_stream_success_(false)
	, b_abort_(false)
	, b_got_eof_(false)
	, b_use_tcp_(false)
	, b_no_buffer_(false)
	, b_re_play_(false)
	, b_notify_closed_(false)
	, n_reconnect_time_(0)
	, n_stat_time_(0)
	, n_net_aud_band_(0)
	, n_net_vid_band_(0)
	, n_conn_pkt_time_(0)
	, n_vid_fps_(0)
	, n_vid_width_(0)
	, n_vid_height_(0)
	, video_dec_ctx_(NULL)
	, audio_dec_ctx_(NULL)
	, avframe_(NULL)
	, audio_convert_ctx_(NULL)
	, p_resamp_buffer_(NULL)
	, n_resmap_size_(0)
	, n_sample_hz_(0)
	, n_channels_(0)
	, p_audio_sample_(NULL)
	, n_audio_size_(0)
	, n_out_sample_hz_(0)
	, p_aud_sonic_(NULL)
	, find_type_6_(NULL)
	, n_type_6_(0)
{
	p_audio_sample_ = new char[SAMPLE_SIZE];

	FFContex::Inst();
}

ARFFPlayer::~ARFFPlayer()
{
	StopTask();

	delete[] p_audio_sample_;
}

int ARFFPlayer::Timeout()
{
	/*if (read_pkt_time_ != 0 && read_pkt_time_ <= rtc::Time32()) {
		return EAGAIN;
	}*/
	if ((n_conn_pkt_time_ != 0 &&n_conn_pkt_time_ <= rtc::Time32()) || b_abort_) {
		return AVERROR_EOF;
	}

	return 0;
}

//* For rtc::Thread
void ARFFPlayer::Run()
{
	int ret = 0;

	while (b_running_) {
		if (b_re_play_) {
			b_re_play_ = false;
			CloseFFDecode();
			n_seek_to_ = 0;
			n_cur_aud_duration_ = 0;
			n_cur_vid_duration_ = 0;
			n_reconnect_time_ = rtc::Time32();
		}
		if (fmt_ctx_ == NULL) {
			if (n_reconnect_time_ != 0 && n_reconnect_time_ <= rtc::Time32()) {
				n_reconnect_time_ = rtc::Time32() + 1500;
			}
			else {
				rtc::Thread::SleepMs(1);
				continue;
			}
			OpenFFDecode();
			//n_seek_to_ = 0;	//@Bug - ���ﲻ����0������seek����ʧЧ
			n_stat_time_ = 0;
			if (fmt_ctx_ != NULL) {
				if (!b_open_stream_success_) {
					b_open_stream_success_ = true;
					callback_.OnArPlyOK(this);
					if (n_all_duration_ > 0 && (n_audio_stream_idx_ != -1 || n_video_stream_idx_ != -1)) {
						callback_.OnArPlyVodProcess(this, n_all_duration_, 0, 0);
					}
				}
				n_stat_time_ = rtc::Time32() + 1000;
				if (n_seek_to_ != 0) {
					//���뵥λ תΪ ΢�뵥λ
					int64_t seek = milliseconds_to_fftime(n_seek_to_ * 1000);
					n_seek_to_ = 0;
					int ret = av_seek_frame(fmt_ctx_, -1, seek, AVSEEK_FLAG_BACKWARD);
					n_stat_time_ = rtc::Time32() + 1000;
				}
			}
			else {
				if (!b_open_stream_success_) {
					CloseFFDecode();
					n_reconnect_time_ = 0;
					if (user_set_.b_repeat_ || user_set_.n_repeat_count_ > 0) {
						if (user_set_.n_repeat_count_ > 0) {
							user_set_.n_repeat_count_--;
						}
						n_reconnect_time_ = rtc::Time32();
					}
					if (n_reconnect_time_ == 0) {
						callback_.OnArPlyClose(this, -1);	// �޷���Դ
						b_notify_closed_ = true;
					}
				}
			}
		}
		else {
			ReadThreadProcess();
		}

		if (n_stat_time_ != 0 && n_stat_time_ <= rtc::Time32()) {
			n_stat_time_ = rtc::Time32() + 1000;
			//callback_.OnArPlyStatus(lst_size * 20, n_net_aud_band_*(8 + 1));
			callback_.OnArPlyStatistics(this, n_vid_width_, n_vid_height_, n_vid_fps_, n_net_aud_band_, n_net_vid_band_);

			if (n_all_duration_ > 0) {
				if (n_audio_stream_idx_ != -1 || n_video_stream_idx_ != -1) {
					int nAudCacheTime = FFBuffer::AudioCacheTime();
					int nVidCacheTime = FFBuffer::VideoCacheTime();
					callback_.OnArPlyVodProcess(this, n_all_duration_, n_cur_vid_duration_> n_cur_aud_duration_? n_cur_vid_duration_ : n_cur_aud_duration_, nVidCacheTime> nAudCacheTime ? nVidCacheTime: nAudCacheTime);
				}
			}
			n_vid_fps_ = 0;
			n_net_aud_band_ = 0;
			n_net_vid_band_ = 0;
		}
		rtc::Thread::SleepMs(5);
	}

	CloseFFDecode();
	if (!b_notify_closed_) {
		callback_.OnArPlyClose(this, b_open_stream_success_ ? 0 : -1);
	}
}

bool ARFFPlayer::ReadThreadProcess()
{
	int ii = 0;
	bool needSeek = false;
	if (n_seek_to_ != 0) {
		if (n_all_duration_ > 0) {
			// ��ת���ķ��� , ��ת������ʱ�������Ĺؼ�֡λ��
			if (!b_got_eof_) {
				//���뵥λ תΪ ΢�뵥λ
				int64_t seek = milliseconds_to_fftime(n_seek_to_ * 1000);
				n_seek_to_ = 0;
				int ret = av_seek_frame(fmt_ctx_, -1, seek, AVSEEK_FLAG_BACKWARD);
				n_stat_time_ = rtc::Time32() + 1000;
			}
			FFBuffer::DoClear();
			needSeek = true;
		}
		else {
			n_seek_to_ = 0;
		}
	}
	if (b_got_eof_)
	{
		if (FFBuffer::BufferIsEmpty()) {
			CloseFFDecode();

			n_reconnect_time_ = 0;
			if (needSeek || user_set_.b_repeat_ || user_set_.n_repeat_count_ > 0) {
				if (user_set_.n_repeat_count_ > 0) {
					user_set_.n_repeat_count_--;
				}
				n_reconnect_time_ = rtc::Time32();
			}
			if (n_reconnect_time_ == 0) {
				callback_.OnArPlyClose(this, 1);
				b_notify_closed_ = true;
			}
		}
		else if (!FFBuffer::IsPlaying()) {
			//* �������EOF�˲��һ�������ֹͣ���ţ���ʱӦ����ջ��壬�����޷���������
			FFBuffer::DoClear();
		}
		return false;
	}
	while (FFBuffer::NeedMoreData())
	{
		if (fmt_ctx_ != NULL) {
			int ret = 0;
			ii++;
			{
				AVPacket *packet = new AVPacket();
				av_init_packet(packet);
				packet->data = NULL;
				packet->size = 0;
				//n_conn_pkt_time_ = rtc::Time32() + 15000;
				n_conn_pkt_time_ = 0;	// ��ȡ���ݵ�ʱ�����ó�ʱ������m3u8��һֱ�ȴ������ϻ���ɲ��ſ���
				/* read frames from the file */
				if ((ret = av_read_frame(fmt_ctx_, packet)) >= 0) {
					if (packet->stream_index == n_video_stream_idx_) {
						int64_t pts = 0;
						n_net_vid_band_ += packet->size;
						if (packet->flags & AV_PKT_FLAG_KEY) {

						}
						if (packet->dts != 0) {
							if (packet->dts == AV_NOPTS_VALUE) {
								pts = 0;
							}
							else {
								pts = av_rescale_q(packet->dts, vstream_timebase_, TIMEBASE_MS);
							}
						}
						else {
							if (packet->pts == AV_NOPTS_VALUE) {
								pts = 0;
							}
							else {
								pts = av_rescale_q(packet->pts, vstream_timebase_, TIMEBASE_MS);
							}
						}
						if (b_no_buffer_) {
							OnBufferDecodeVideoData(packet);
							av_packet_unref(packet);
							delete packet;
						}
						else {
							FFBuffer::RecvVideoData(packet, pts, pts, av_rescale_q(packet->duration, vstream_timebase_, TIMEBASE_MS));
						}
					}
					else if (packet->stream_index == n_audio_stream_idx_) {
						int64_t pts = 0;
						n_net_aud_band_ += packet->size;
						if (packet->dts != 0) {
							if (packet->dts == AV_NOPTS_VALUE) {
								pts = 0;
							}
							else {
								pts = av_rescale_q(packet->dts, astream_timebase_, TIMEBASE_MS);
							}
						}
						else {
							if (packet->pts == AV_NOPTS_VALUE) {
								pts = 0;
							}
							else {
								pts = av_rescale_q(packet->pts, astream_timebase_, TIMEBASE_MS);
							}
						}
						if (b_no_buffer_) {
							OnBufferDecodeAudioData(packet);
							av_packet_unref(packet);
							delete packet;
						}
						else {
							FFBuffer::RecvAudioData(packet, pts, pts, av_rescale_q(packet->duration, astream_timebase_, TIMEBASE_MS));
						}
					}
				}
				else {
					delete packet;
					if (ret == AVERROR_EOF) {
						b_got_eof_ = true;
						return false;
					}
				}
			}

			if (ii > 5) {//* �߳�ִ��ʱ�� (windows����ʵʱ����ϵͳ)����sleep ��������Ҳ����15ms���ҡ����Ե��ζ�ȡ��Ҫ��һ�㣬����ᵼ��FFMpeg�Ļ��岻�ܼ�ʱ���������¿���
				break;
			}
		}
		else {
			break;
		}
	}
	return true;
}

int ARFFPlayer::StartTask(const char*strUrl)
{
	if (!b_running_) {
		str_play_url_ = strUrl;
		b_abort_ = false;
		n_cur_aud_duration_ = 0;
		n_cur_vid_duration_ = 0;
		b_running_ = true;
		user_set_.b_paused_ = false;
		n_reconnect_time_ = rtc::Time32();
		rtc::Thread::SetName("ARFFPlayer", this);
		rtc::Thread::Start();
	}

	return 0;
}

void ARFFPlayer::RunOnce()
{
	FFBuffer::DoTick();

	while (callback_.OnArPlyNeedMoreAudioData(this)) {
		if (!FFBuffer::DoDecodeAudio()) {
			break;
		}
	}
	while (callback_.OnArPlyNeedMoreVideoData(this)) {
		if (!FFBuffer::DoDecodeVideo()) {
			break;
		}
	}
}
void ARFFPlayer::Play()
{
	if (user_set_.b_paused_) {
		user_set_.b_paused_ = false;
		callback_.OnArPlyResume(this);
	}
}
void ARFFPlayer::Pause()
{
	if (!user_set_.b_paused_) {
		user_set_.b_paused_ = true;
		callback_.OnArPlyPaused(this);
	}
}
void ARFFPlayer::StopTask()
{
	if (b_running_) {
		b_running_ = false;
		b_abort_ = true;
		n_reconnect_time_ = 0;
		user_set_.b_repeat_ = false;
		rtc::Thread::Stop();
	}

	FFBuffer::DoClear();

	if (p_aud_sonic_ != NULL) {
		sonicDestroyStream(p_aud_sonic_);
		p_aud_sonic_ = NULL;
	}
}

void ARFFPlayer::SetAudioEnable(bool bAudioEnable)
{
	user_set_.b_audio_enabled_ = bAudioEnable;
}
void ARFFPlayer::SetVideoEnable(bool bVideoEnable)
{
	user_set_.b_video_enabled_ = bVideoEnable;
}
void ARFFPlayer::SetRepeat(bool bEnable)
{
	user_set_.b_repeat_ = bEnable;
}
void ARFFPlayer::SetUseTcp(bool bUseTcp)
{
	b_use_tcp_ = bUseTcp;
}
void ARFFPlayer::SetNoBuffer(bool bNoBuffer)
{
	b_no_buffer_ = bNoBuffer;
}
void ARFFPlayer::SetRepeatCount(int loopCount)
{
	user_set_.n_repeat_count_ = 0;
	if (loopCount < 0) {
		SetRepeat(true);
	}
	else if(loopCount >= 0){
		user_set_.n_repeat_count_ = loopCount;
	}
}
void ARFFPlayer::SeekTo(int nSeconds)
{
	n_seek_to_ = nSeconds;
}
void ARFFPlayer::SetSpeed(float fSpeed)
{
	user_set_.f_speed_ = fSpeed;
	FFBuffer::ResetTime();
}
void ARFFPlayer::SetVolume(float fVolume)
{
	user_set_.f_aud_volume_ = fVolume;
}
void ARFFPlayer::EnableVolumeEvaluation(int32_t intervalMs)
{
	user_set_.n_volume_evaluation_interval_ms_ = intervalMs;
	user_set_.n_volume_evaluation_used_ms_ = 0;
}
int ARFFPlayer::GetTotalDuration()
{
	return n_all_duration_;
}
int ARFFPlayer::GetCurAudDuration()
{
	return n_cur_aud_duration_;
}
int ARFFPlayer::GetCurVidDuration()
{
	return n_cur_vid_duration_;
}
void ARFFPlayer::RePlay()
{
	b_re_play_ = true;
}
void ARFFPlayer::Config(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold)
{
	FFBuffer::SetPlaySetting(bAuto, nCacheTime, nMinCacheTime, nMaxCacheTime, nVideoBlockThreshold);
}
void ARFFPlayer::selectAudioTrack(int index)
{
	int total = 0;
	AVFormatContext *ic = fmt_ctx_;
	int start_index = 0, stream_index = 0;
	AVStream *st;
	int codec_type = AVMEDIA_TYPE_AUDIO;

	if (codec_type == AVMEDIA_TYPE_VIDEO)
		start_index = n_video_stream_idx_;
	else if (codec_type == AVMEDIA_TYPE_AUDIO)
		start_index = n_audio_stream_idx_;
	/*else
	 start_index = is->subtitle_stream;
	 if (start_index < (codec_type == AVMEDIA_TYPE_SUBTITLE ? -1 : 0))
	 return;*/

	for (stream_index = 0; stream_index < ic->nb_streams; stream_index++)
	{
		st = ic->streams[stream_index];
		if (st->codecpar->codec_type == codec_type) {
			/* check that parameters are OK */
			switch (codec_type) {
			case AVMEDIA_TYPE_AUDIO: {
				if (st->codecpar->sample_rate != 0 && st->codecpar->channels != 0) {
					total++;
				}

				if (total == index)
				{
					//stream_component_close(ffp, start_index);
					//������������tracksNum������ĸ�����stream_index���ڼ�������
					//ffp_set_stream_selected(ffp, tracksNum, stream_index);
					return;
				}
			}break;
			}
		}
	}

	return;
}
int ARFFPlayer::getAudioTrackCount()
{
	return n_total_track_;
}

//* For FFBuffer
void ARFFPlayer::OnBufferDecodeAudioData(AVPacket* aud_packet)
{
	AVPacket &pkt = *aud_packet;
	uint8_t*ptr = pkt.data;
	int len = pkt.size;
	int frameFinished = 0;

	{
#ifdef WEBRTC_80
		webrtc::MutexLock l(&cs_ff_ctx_);
#else
		rtc::CritScope l(&cs_ff_ctx_);
#endif
		if (audio_dec_ctx_ != NULL) {
#if 0
			int ret = avcodec_decode_audio4(audio_dec_ctx_, avframe_, &frameFinished, &pkt);
#else
			int ret = avcodec_send_packet(audio_dec_ctx_, &pkt);
			if (ret < 0) {
				RTC_LOG(LS_ERROR) << "avcodec_send_packet error: " << ret;
			}
			else {
				ret = avcodec_receive_frame(audio_dec_ctx_, avframe_);
				if (ret < 0) {
					RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << ret;
				}
				else {
					frameFinished = 1;
				}
			}
#endif 
			if (ret >= 0) {
				int64_t pts = 0;
				if (frameFinished) {
					int out_channels = av_get_channel_layout_nb_channels(audio_dec_ctx_->channel_layout);
					int need_size = (n_out_sample_hz_* out_channels * sizeof(int16_t)) / 100;
					//avframe_->pts = av_rescale_q(av_frame_get_best_effort_timestamp(avframe_), astream_timebase_, TIMEBASE_MS);
					avframe_->pts = av_rescale_q(avframe_->best_effort_timestamp, astream_timebase_, TIMEBASE_MS);
					//RTC_LOG(LS_ERROR) << "Audio pts: " << pkt.pts;
					pts = avframe_->pts;
					/* if a frame has been decoded, output it */
					int data_size = av_get_bytes_per_sample(audio_dec_ctx_->sample_fmt);
					if (data_size > 0) {
						int samples = swr_convert(audio_convert_ctx_, &p_resamp_buffer_, n_resmap_size_, (const uint8_t **)avframe_->data, avframe_->nb_samples);
						if (samples > 0) {
							int resampled_data_size = samples * out_channels  * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//ÿ���������� x ������ x ÿ�������ֽ���   

							memcpy(p_audio_sample_ + n_audio_size_, p_resamp_buffer_, resampled_data_size);
							pts -= (n_audio_size_ * 10) / need_size;
							n_audio_size_ += resampled_data_size;
						}
					}
					{
						//��ȡ��ǰ��Ƶ����Բ���ʱ�� , ��� : ���Ӳ��ſ�ʼ�����ڵ�ʱ��
						//  ��ֵ���������� , �� pts ֵ����ͬ��
						//  ��ֵ�� pts ���Ӿ�׼ , �ο��˸������Ϣ
						//  ת������ : ����Ҫע�� pts ��Ҫת�� �� , ��Ҫ���� time_base ʱ�䵥λ
						//  ���� av_q2d �ǽ� AVRational תΪ double ����
						double audio_best_effort_timestamp_second = avframe_->best_effort_timestamp * av_q2d(astream_timebase_);
						n_cur_aud_duration_ = audio_best_effort_timestamp_second * 1000;
					}
					av_frame_unref(avframe_);

					while (n_audio_size_ >= need_size) {
						GotAudioFrame(p_audio_sample_, need_size, n_out_sample_hz_, out_channels, pts);
						pts += 10;
						n_audio_size_ -= need_size;
						if (n_audio_size_ > 0) {
							memmove(p_audio_sample_, p_audio_sample_ + need_size, n_audio_size_);
						}
					}
				}
			}
			else {
				char errBuf[1024] = { 0 };
				av_strerror(ret, errBuf, 1024);
			}
		}
	}
}
void ARFFPlayer::OnBufferDecodeVideoData(AVPacket* vid_packet)
{
	{
		int64_t pts = av_rescale_q(vid_packet->dts, vstream_timebase_, TIMEBASE_MS);
		//printf("Time: %d pts:%lld \r\n", rtc::Time32(), pts);
		if (vid_packet->flags & AV_PKT_FLAG_KEY) {
			ParseVideoSei((char*)vid_packet->data, vid_packet->size, pts);
		}
	}
#ifdef WEBRTC_80
	webrtc::MutexLock l(&cs_ff_ctx_);
#else
	rtc::CritScope l(&cs_ff_ctx_);
#endif
	if (video_dec_ctx_ != NULL) {
		{//* Decode?
			int frameFinished = 0;
#if 0
			int ret = avcodec_decode_video2(video_dec_ctx_, avframe_, &frameFinished, vid_packet);
#else
			int ret = avcodec_send_packet(video_dec_ctx_, vid_packet);
			if (ret < 0) {
				RTC_LOG(LS_ERROR) << "avcodec_send_packet error: " << ret;
			}
			else {
				ret = avcodec_receive_frame(video_dec_ctx_, avframe_);
				if (ret < 0) {
					RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << ret;
				}
				else {
					frameFinished = 1;
				}
			}
#endif
			if (ret >= 0) {
				if (frameFinished) {
					//avframe_->pts = av_rescale_q(av_frame_get_best_effort_timestamp(avframe_), vstream_timebase_, TIMEBASE_MS);
					avframe_->pts = av_rescale_q(avframe_->best_effort_timestamp, vstream_timebase_, TIMEBASE_MS);
					if (b_no_buffer_) {
						avframe_->pts = 0;
					}
					{
						//��ȡ��ǰ�������Բ���ʱ�� , ��� : ���Ӳ��ſ�ʼ�����ڵ�ʱ��
						//  ��ֵ���������� , �� pts ֵ����ͬ��
						//  ��ֵ�� pts ���Ӿ�׼ , �ο��˸������Ϣ
						//  ת������ : ����Ҫע�� pts ��Ҫת�� �� , ��Ҫ���� time_base ʱ�䵥λ
						//  ���� av_q2d �ǽ� AVRational תΪ double ����
						double video_best_effort_timestamp_second = avframe_->best_effort_timestamp * av_q2d(vstream_timebase_);
						n_cur_vid_duration_ = video_best_effort_timestamp_second * 1000;
					}
					n_vid_fps_++;
					n_vid_width_ = avframe_->width;
					n_vid_height_ = avframe_->height;
					callback_.OnArPlyVideo(this, avframe_->format, avframe_->width, avframe_->height, avframe_->data, avframe_->linesize, avframe_->pts);
				}
			}
		}
	}
}
void ARFFPlayer::OnBufferStatusChanged(PlayStatus playStatus)
{
	if (playStatus == PS_Caching) {
		callback_.OnArPlyCacheing(this, user_set_.b_audio_enabled_, user_set_. b_video_enabled_);
	}
	else if (playStatus == PS_Playing) {
		callback_.OnArPlyPlaying(this, user_set_.b_audio_enabled_, user_set_.b_video_enabled_);
	}
}
bool ARFFPlayer::OnBufferGetPuased()
{
	return user_set_.b_paused_ || (!user_set_.b_audio_enabled_ && !user_set_.b_video_enabled_);
}
float ARFFPlayer::OnBufferGetSpeed()
{
	if (n_all_duration_ <= 0) {// �����ǵ㲥���ܴ�����
		return 1.0;
	}
	return user_set_.f_speed_;
}

void ARFFPlayer::GotAudioFrame(char* pdata, int len, int sample_hz, int channels, int64_t timestamp)
{
	if (e_audio_dual_mono_mode_ == AUDIO_DUAL_MONO_L) {
		audio_track_change((uint8_t*)pdata, len, 1, 16);
	}
	else if (e_audio_dual_mono_mode_ == AUDIO_DUAL_MONO_R) {
		audio_track_change((uint8_t*)pdata, len, 2, 16);
	}
	if (user_set_.f_aud_volume_ != 1.0 || user_set_.f_speed_ != 1.0) {
		if (p_aud_sonic_ == NULL) {
			p_aud_sonic_ = sonicCreateStream(sample_hz, channels);
		}
	}
	else {
		if (p_aud_sonic_ != NULL) {
			sonicDestroyStream(p_aud_sonic_);
			p_aud_sonic_ = NULL;
		}
	}
	if (user_set_.n_volume_evaluation_interval_ms_ > 0) {
		user_set_.n_volume_evaluation_used_ms_ += 10;
		if (user_set_.n_volume_evaluation_used_ms_ >= user_set_.n_volume_evaluation_interval_ms_) {
			user_set_.n_volume_evaluation_used_ms_ = 0;
			int max_abs = WebRtcSpl_MaxAbsValueW16_I((int16_t*)(pdata), len / sizeof(int16_t));
			max_abs = (max_abs * 100) / 32767;
			callback_.OnArPlyVolumeUpdate(this, max_abs);
		}
	}
	// �����ǵ㲥���ܴ�����
	if (p_aud_sonic_ != NULL) {
		if (n_all_duration_ > 0 && user_set_.f_speed_ != 1.0) {
			sonicSetSpeed(p_aud_sonic_, user_set_.f_speed_);
		}
		else {
			sonicSetSpeed(p_aud_sonic_, 1.0);
		}
		sonicSetVolume(p_aud_sonic_, user_set_.f_aud_volume_);
		int audFrame10ms = len / sizeof(short) / channels;
		sonicWriteShortToStream(p_aud_sonic_, (short*)pdata, audFrame10ms);
		int ava = sonicSamplesAvailable(p_aud_sonic_);
		//RTC_LOG(LS_INFO) << "sonicWriteShortToStream: " << audFrame10ms << " ava: " << ava;
		while (sonicSamplesAvailable(p_aud_sonic_) >= audFrame10ms) {
			memset(pdata, 0, len);
			int rd = sonicReadShortFromStream(p_aud_sonic_, (short*)pdata, audFrame10ms);
			//RTC_LOG(LS_INFO) << "sonicReadShortFromStream: " << rd;
			if (rd > 0) {
				callback_.OnArPlyAudio(this, pdata, sample_hz, channels, timestamp);
			}
			else {
				break;
			}
		}
	}
	else {
		callback_.OnArPlyAudio(this, pdata, sample_hz, channels, timestamp);
	}
}
bool ARFFPlayer::GotNaluPacket(const unsigned char* pData, int nLen, int64_t pts)
{
	int limLen = 4;
	unsigned char* ptr = (unsigned char*)pData;
	if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01)
	{
		limLen = 3;
	}
	int len = nLen;
	int nType = (ptr[limLen] & 0x1f);
	if (nType == 7 || nType == 5 || nType == 1) {
		char* pH264Raw = NULL;
		int dataLen = nLen;
		if (nType == 7 || nType == 1) {
			if (limLen == 3) {
				pH264Raw = new char[nLen + 1];
				dataLen = nLen + 1;
				pH264Raw[0] = 0x0;
				memcpy(pH264Raw + 1, ptr, nLen);
			}
			else {
				pH264Raw = new char[nLen];
				memcpy(pH264Raw, ptr, nLen);
			}
		}
		else {
			if (video_dec_ctx_->extradata != NULL && video_dec_ctx_->extradata_size > 0) {
				if (video_dec_ctx_->extradata[0] == 0x0 && video_dec_ctx_->extradata[1] == 0x0 && video_dec_ctx_->extradata[2] == 0x0 && video_dec_ctx_->extradata[3] == 0x1) {
					dataLen = video_dec_ctx_->extradata_size;
					pH264Raw = new char[dataLen];
					memcpy(pH264Raw, video_dec_ctx_->extradata, dataLen);
				}
				else {
					int ppsLen = (int)(video_dec_ctx_->extradata[6] << 8) + video_dec_ctx_->extradata[7];
					int spsLen = video_dec_ctx_->extradata_size - 6 - 2 - ppsLen - 3;
					dataLen += ppsLen + 4 + spsLen + 4;
					pH264Raw = new char[dataLen];
					char naluHdr[4] = { 0x00, 0x00, 0x00, 0x01 };

					int nPtr = 0;
					memcpy(pH264Raw, naluHdr, 4);
					nPtr += 4;
					memcpy(pH264Raw + nPtr, video_dec_ctx_->extradata + 8, ppsLen);
					nPtr += ppsLen;
					memcpy(pH264Raw + nPtr, naluHdr, 4);
					nPtr += 4;
					memcpy(pH264Raw + nPtr, video_dec_ctx_->extradata + 8 + ppsLen + 3, spsLen);
					nPtr += spsLen;
					memcpy(pH264Raw + nPtr, ptr, nLen);
					nPtr += nLen;
				}

			}
		}

		if (pH264Raw != NULL) {
			int nType = pH264Raw[4] & 0x1f;
			callback_.OnArPlyRawVideo(this, pH264Raw, dataLen, nType == 7, pts);

			delete[] pH264Raw;
			pH264Raw = NULL;
		}

		return true;
	}
	else {
		if (nType == 6) {
			if (find_type_6_ == NULL) {
				find_type_6_ = (char*)ptr;
				n_type_6_ = len;
			}
		}
		ptr += limLen;
		len -= limLen;
		while (len > limLen) {
			if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01) {
				return GotNaluPacket(ptr, len, pts);
				break;
			}
			else if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01) {
				return GotNaluPacket(ptr, len, pts);
				break;
			}
			else {
				ptr++;
				len--;
			}
		}
	}

	return false;
}
void ARFFPlayer::GotVideoPacket(const unsigned char* pData, int nLen, int64_t pts)
{
	/*
	put_byte(pb, 1);      // version
	put_byte(pb, sps[1]); // profile
	put_byte(pb, sps[2]); // profile compat
	put_byte(pb, sps[3]); // level
	put_byte(pb, 0xff);   // 6 bits reserved (111111) + 2 bits nal size length - 1 (11)
	put_byte(pb, 0xe1);   // 3 bits reserved (111) + 5 bits number of sps (00001)
	put_be16(pb, sps_size);
	put_buffer(pb, sps, sps_size);

	put_byte(pb, 1);      // number of pps
	put_be16(pb, pps_size);
	put_buffer(pb, pps, pps_size);
	*/

	unsigned char* ptr = (unsigned char*)pData;
	if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01) {
		find_type_6_ = NULL;
		n_type_6_ = 0;
		if (!GotNaluPacket(pData, nLen, pts))
		{
			if (find_type_6_ != NULL) {
				char* pH264Raw = NULL;
				pH264Raw = new char[n_type_6_];
				memcpy(pH264Raw, find_type_6_, n_type_6_);

				if (pH264Raw != NULL) {
					int nType = pH264Raw[4] & 0x1f;
					callback_.OnArPlyRawVideo(this, pH264Raw, n_type_6_, nType == 7, pts);

					delete[] pH264Raw;
					pH264Raw = NULL;
				}
			}
		}
		return;
	}

	int nPtr = 0;
	int nH264Size = nLen + 32;	// + 32 ��ֹ�ڴ治��
	if (video_dec_ctx_->extradata_size > 0) {
		nH264Size += video_dec_ctx_->extradata_size;
	}
	char* pH264Raw = new char[nH264Size];
	int nH264Len = 0;
	while (nPtr < nLen) {
		// 1. first we check NAL's size, valid size should less than 192K = 0x030000
		if (ptr[0] != 0x00 || ptr[1] >= 0x03)
		{
			delete[]pH264Raw;
			pH264Raw = NULL;
			return;
		}
		unsigned int vpkg_len = (ptr[1] << 16) + (int)(ptr[2] << 8) + ptr[3];	//

		// 2. check NAL header including NAL start and type,
		//    only nal_unit_type = 1 and 5 are selected
		//    nal_ref_idc > 0
		if ((ptr[4] != 0x21)
			&& (ptr[4] != 0x25)
			&& (ptr[4] != 0x41)
			&& (ptr[4] != 0x45)
			&& (ptr[4] != 0x61)
			&& (ptr[4] != 0x65)) {
			ptr += (4 + vpkg_len);
			nPtr += (4 + vpkg_len);
		}
		else {
			int nType = ptr[4] & 0x1f;
			if (nType == 5) {
				if (video_dec_ctx_->extradata != NULL && video_dec_ctx_->extradata_size > 0) {
					char naluHdr[4] = { 0x00, 0x00, 0x00, 0x01 };

					int nPtr = nH264Len;
					if (nH264Len == 0) {
						int ppsLen = (int)(video_dec_ctx_->extradata[6] << 8) + video_dec_ctx_->extradata[7];
						int spsLen = video_dec_ctx_->extradata_size - 6 - 2 - ppsLen - 3;

						memcpy(pH264Raw + nPtr, naluHdr, 4);
						nPtr += 4;
						memcpy(pH264Raw + nPtr, video_dec_ctx_->extradata + 8, ppsLen);
						nPtr += ppsLen;
						memcpy(pH264Raw + nPtr, naluHdr, 4);
						nPtr += 4;
						memcpy(pH264Raw + nPtr, video_dec_ctx_->extradata + 8 + ppsLen + 3, spsLen);
						nPtr += spsLen;
					}

					memcpy(pH264Raw + nPtr, naluHdr, 4);
					nPtr += 4;
					memcpy(pH264Raw + nPtr, ptr + 4, vpkg_len);
					nPtr += vpkg_len;

					nH264Len = nPtr;
				}
			}
			else if (nType != 9) {
				char naluHdr[4] = { 0x00, 0x00, 0x00, 0x01 };

				int nPtr = nH264Len;
				memcpy(pH264Raw + nPtr, naluHdr, 4);
				nPtr += 4;
				memcpy(pH264Raw + nPtr, ptr + 4, vpkg_len);
				nPtr += vpkg_len;

				nH264Len = nPtr;
			}
			//LOG(LS_ERROR) << "Pkt type: " << (ptr[4] & 0x1f) << " len: " << vpkg_len;
			//callback_.OnH264RawData((char*)ptr, 4 + vpkg_len);
			ptr += (4 + vpkg_len);
			nPtr += (4 + vpkg_len);
		}
	}

	if (nH264Len > 0) {
		int nType = pH264Raw[4] & 0x1f;
		callback_.OnArPlyRawVideo(this, pH264Raw, nH264Len, nType == 7, pts);
	}

	if (pH264Raw != NULL) {
		delete[] pH264Raw;
		pH264Raw = NULL;
	}
}

void ARFFPlayer::ParseVideoSei(char* pData, int nLen, int64_t pts)
{
	char* ptr = pData;
	if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01) {
		int limLen = 0;
		int ii = 0;
		char* ptrFind6 = NULL;
		while (ii <= nLen - 4) {
			limLen = 0;
			if (ptr[ii] == 0 && ptr[ii + 1] == 0 && ptr[ii + 2] == 0 && ptr[ii + 3] == 1) {
				limLen = 4;
			}
			else if (ptr[ii] == 0 && ptr[ii + 1] == 0 && ptr[ii + 2] == 1) {
				limLen = 3;
			}
			if (limLen > 0) {
				ii += limLen;
				int nType = ptr[ii] & 0x1f;
				if (nType == 6) {
					ptrFind6 = ptr + ii - limLen;
				}
				else {
					char* ptrFindN = ptr + ii - limLen;
					if (ptrFind6 != NULL) {
						callback_.OnArPlySeiData(this, ptrFind6, ptrFindN - ptrFind6, pts);
						break;
					}
				}
			}
			else {
				ii++;
			}
		}
		if (ptrFind6 != NULL) {
			callback_.OnArPlySeiData(this, ptrFind6, (pData + nLen) - ptrFind6, pts);
		}
	}
	else {
		int nPtr = 0;
		while (nPtr < nLen) {
			// 1. first we check NAL's size, valid size should less than 192K = 0x030000
			if (ptr[0] != 0x00 || ptr[1] >= 0x03)
			{
				return;
			}
			int n1 = (int)(ptr[1] & 0xff) << 16;
			int n2 = (int)(ptr[2] & 0xff) << 8;
			int n3 = (int)(ptr[3] & 0xff);
			unsigned int vpkg_len = n1 + n2 + n3;	//

			// 2. check NAL header including NAL start and type,
			//    only nal_unit_type = 1 and 5 are selected
			//    nal_ref_idc > 0
			int nType = ptr[4] & 0x1f;
			if (nType == 6) {
				char naluHdr[4] = { 0x00, 0x00, 0x00, 0x01 };

				int nRawLen = 0;
				char* pH264Raw = new char[vpkg_len + 4];
				memcpy(pH264Raw + nRawLen, naluHdr, 4);
				nRawLen += 4;
				memcpy(pH264Raw + nRawLen, ptr + 4, vpkg_len);
				nRawLen += vpkg_len;

				callback_.OnArPlySeiData(this, pH264Raw, vpkg_len + 4, pts);
				break;
			}
			//LOG(LS_ERROR) << "Pkt type: " << (ptr[4] & 0x1f) << " len: " << vpkg_len;
			//callback_.OnH264RawData((char*)ptr, 4 + vpkg_len);
			ptr += (4 + vpkg_len);
			nPtr += (4 + vpkg_len);
		}
	}
}

void ARFFPlayer::OpenFFDecode()
{
#ifdef WEBRTC_80
	webrtc::MutexLock l(&cs_ff_ctx_);
#else
	rtc::CritScope l(&cs_ff_ctx_);
#endif
	if (fmt_ctx_ == NULL) {
		fmt_ctx_ = avformat_alloc_context();
		fmt_ctx_->interrupt_callback.callback = custom_interrupt_callback;
		fmt_ctx_->interrupt_callback.opaque = this;

		int ret = 0;
		/* open input file, and allocate format context */
		n_conn_pkt_time_ = rtc::Time32() + 10000;
		AVDictionary* options = NULL;
		av_dict_set(&options, "nobuffer", "1", 0);
		if (str_play_url_.find("rtmp://") != std::string::npos) {
			av_dict_set(&options, "timeout", NULL, 0);
		}
		if (str_play_url_.find("rtsp://") != std::string::npos) {
			if (b_use_tcp_) {
				av_dict_set(&options, "rtsp_transport", "tcp", 0);
			}
			else {
				av_dict_set(&options, "rtsp_transport", "udp", 0);
			}
			av_dict_set(&options, "timeout", NULL, 0);
		}
		if ((ret = avformat_open_input(&fmt_ctx_, str_play_url_.c_str(), NULL, &options)) < 0) {
			char strErr[1024];
			av_strerror(ret, strErr, 1024);
			printf("Could not open source (%d) url %s\n", ret, str_play_url_.c_str());
			return;
		}
		n_all_duration_ = fftime_to_milliseconds(fmt_ctx_->duration);	//duration��λ��us��ת��Ϊ����
		if (n_all_duration_ < 0) {
			n_all_duration_ = 0;
		}
        fmt_ctx_->probesize = 128 *1024;
        fmt_ctx_->max_analyze_duration = 1 * AV_TIME_BASE;
		//��avformat_find_stream_info�����У����ж�AVFMT_FLAG_NOBUFFER�����Ƿ���ӵ�buffer��
		//fmt_ctx_->flags |= AVFMT_FLAG_NOBUFFER;
		//fmt_ctx_->flags |= AVFMT_FLAG_DISCARD_CORRUPT;
		/* retrieve stream information */
		if (avformat_find_stream_info(fmt_ctx_, NULL) < 0) {
			printf("Could not find stream information\n");
			avformat_close_input(&fmt_ctx_);
			fmt_ctx_ = NULL;
			return;
		}
		{// Get audio Track total
			int total = 0;
			int codec_type = AVMEDIA_TYPE_AUDIO;
			for (int stream_index = 0; stream_index < fmt_ctx_->nb_streams; stream_index++)
			{
				AVStream* st = fmt_ctx_->streams[stream_index];
				if (st->codecpar->codec_type == codec_type) {
					/* check that parameters are OK */
					switch (codec_type) {
					case AVMEDIA_TYPE_AUDIO:
					{
						if (st->codecpar->sample_rate != 0 && st->codecpar->channels != 0) {
							total++;
						}
					}break;
					}
				}
			}
			n_total_track_ = total;
		}

		if (open_codec_context(&n_video_stream_idx_, &video_dec_ctx_, fmt_ctx_, AVMEDIA_TYPE_VIDEO) >= 0) {
			video_stream_ = fmt_ctx_->streams[n_video_stream_idx_];
			vstream_timebase_ = fmt_ctx_->streams[n_video_stream_idx_]->time_base;
		}
        else {
            n_video_stream_idx_ = -1;
        }

		if (open_codec_context(&n_audio_stream_idx_, &audio_dec_ctx_, fmt_ctx_, AVMEDIA_TYPE_AUDIO) >= 0) {
			audio_stream_ = fmt_ctx_->streams[n_audio_stream_idx_];
			astream_timebase_ = fmt_ctx_->streams[n_audio_stream_idx_]->time_base;

			n_sample_hz_ = audio_dec_ctx_->sample_rate;
			n_channels_ = audio_dec_ctx_->channels;
			n_out_sample_hz_ = 48000;// n_sample_hz_
			/*if (audio_dec_ctx_->channel_layout == 0) {
				audio_dec_ctx_->channel_layout = AV_CH_LAYOUT_MONO;
			}*/
			//@Bug: FFMPEG ����WAV ��ȡ��������
			if (audio_dec_ctx_->channels > 0 && audio_dec_ctx_->channel_layout == 0) { //��������û���������֣�����Ҫ������������
				audio_dec_ctx_->channel_layout = av_get_default_channel_layout(audio_dec_ctx_->channels);//������������
			}
			else if (audio_dec_ctx_->channels == 0 && audio_dec_ctx_->channel_layout > 0) {//����������û��������������Ҫ����������
				audio_dec_ctx_->channels = av_get_channel_layout_nb_channels(audio_dec_ctx_->channel_layout);
			}
			//Swr  
			audio_convert_ctx_ = swr_alloc();
			audio_convert_ctx_ = swr_alloc_set_opts(audio_convert_ctx_, audio_dec_ctx_->channel_layout, AV_SAMPLE_FMT_S16, n_out_sample_hz_,
				audio_dec_ctx_->channel_layout, audio_dec_ctx_->sample_fmt, audio_dec_ctx_->sample_rate, 0, NULL);//����Դ��Ƶ������Ŀ����Ƶ���� 
			swr_init(audio_convert_ctx_);
			int frame_size = (audio_dec_ctx_->frame_size == 0 ? 4096 : audio_dec_ctx_->frame_size) * 8;
			n_resmap_size_ = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(audio_dec_ctx_->channel_layout), frame_size, AV_SAMPLE_FMT_S16, 1);//����ת�������ݴ�С  
			p_resamp_buffer_ = (uint8_t *)av_malloc(n_resmap_size_);//�������������  
		}
        else {
            n_audio_stream_idx_ = -1;
        }

		/* dump input information to stderr */
		av_dump_format(fmt_ctx_, 0, str_play_url_.c_str(), 0);

		if (avframe_ == NULL) {
			avframe_ = av_frame_alloc();
		}

		b_got_eof_ = false;
	}
}
void ARFFPlayer::CloseFFDecode()
{
	FFBuffer::DoClear();
	n_reconnect_time_ = 0;

#ifdef WEBRTC_80
	webrtc::MutexLock l(&cs_ff_ctx_);
#else
	rtc::CritScope l(&cs_ff_ctx_);
#endif
	if (video_dec_ctx_ != NULL) {
		avcodec_close(video_dec_ctx_);
		video_dec_ctx_ = NULL;
	}
	if (audio_dec_ctx_ != NULL) {
		avcodec_close(audio_dec_ctx_);
		audio_dec_ctx_ = NULL;
	}

	if (fmt_ctx_ != NULL) {
		avformat_close_input(&fmt_ctx_);
		fmt_ctx_ = NULL;
	}

	if (avframe_ != NULL) {
		av_frame_free(&avframe_);
		avframe_ = NULL;
	}
	if (audio_convert_ctx_ != NULL) {
		swr_free(&audio_convert_ctx_);
		audio_convert_ctx_ = NULL;
	}
}

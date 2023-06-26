/*
*  Copyright (c) 2021 The AnyRTC project authors. All Rights Reserved.
*
*  Please visit https://www.anyrtc.io for detail.
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __I_AR_LIVE2_ENGINE_H__
#define __I_AR_LIVE2_ENGINE_H__
#include <stdint.h>
#include "IArLivePusher.hpp"
#include "IArLivePlayer.hpp"

namespace anyrtc {

class IArLive2EngineObserver
{
public:
	IArLive2EngineObserver(void) {};
	virtual ~IArLive2EngineObserver(void) {};

	
};

class IArLive2Engine
{
protected:
	IArLive2Engine(void) {};
	
public:
	virtual ~IArLive2Engine(void) {};

	/**
		* ��������Ļص���
		*
		* ͨ�����ûص������Լ��� IArLive2Engine ��һЩ�ص��¼���
		* ��������Ƶ���ݡ�����ʹ�����Ϣ�ȡ�
		*
		* @param observer �������Ļص�Ŀ����󣬸�����Ϣ��鿴 {@link IArLive2EngineObserver}
		*/
	virtual int32_t initialize(IArLive2EngineObserver* observer) = 0;

	/**
		* �ͷŶ���
		* @param
		*/
	virtual void release() = 0;

public:
	/// @name ���������� ArLivePusher ʵ��
	/// @{
#ifdef __ANDROID__

/**
	* ���ڶ�̬���� dll ʱ����ȡ ArLivePusher ����ָ�롣
	*
	* @return ���� ArLivePusher �����ָ�룬ע�⣺��Ҫ���� releaseArLivePusher����
	* @param context Android �����ģ��ڲ���תΪ ApplicationContext ����ϵͳ API ����
	* @param mode ����Э�飬RTMP����ROOM
	* @note ���ӿڽ�������Androidƽ̨
	*/
	virtual AR::IArLivePusher* createArLivePusher(void* context) = 0;

	/**
		* ���ڶ�̬���� dll ʱ����ȡ ArLivePlayer ����ָ��
		*
		* @return ���� ArLivePlayer �����ָ�룬ע�⣺����� releaseArLivePlayer ����
		* @param context Android �����ģ��ڲ���תΪ ApplicationContext ����ϵͳ API ����
		* @note ���ӿڽ�������Androidƽ̨
		*/
	virtual AR::IArLivePlayer* createArLivePlayer(void* context) = 0;
#else

/**
	* ���ڶ�̬���� dll ʱ����ȡ ArLivePusher ����ָ�롣
	*
	* @return ���� ArLivePusher �����ָ�룬ע�⣺��Ҫ���� releaseArLivePusher������
	* @param mode ����Э�飬RTMP����ROOM
	* @note ���ӿ�������Windows��Mac��iOSƽ̨
	*/
	virtual AR::IArLivePusher* createArLivePusher() = 0;

	/**
		* ���ڶ�̬���� dll ʱ����ȡ ArLivePlayer ����ָ��
		*
		* @return ���� ArLivePlayer �����ָ�룬ע�⣺����� releaseArLivePlayer ����
		* @note ���ӿ�������Windows��Mac��iOSƽ̨
		*/
	virtual AR::IArLivePlayer* createArLivePlayer() = 0;
#endif

	/**
		* ���� ArLivePusher ����
		*
		* @param pusher ArLivePusher �����ָ��
		*/
	virtual void releaseArLivePusher(AR::IArLivePusher* pusher) = 0;

	/**
		* ���� ArLivePlayer ����
		*
		* @param player ArLivePlayer �����ָ��
		*/
	virtual void releaseArLivePlayer(AR::IArLivePlayer* player) = 0;
	/// @}

public:
	
};

V2_API IArLive2Engine* V2_CALL createArLive2Engine();

};//namespace anyrtc;

#endif	// __I_AR_LIVE2_ENGINE_H__

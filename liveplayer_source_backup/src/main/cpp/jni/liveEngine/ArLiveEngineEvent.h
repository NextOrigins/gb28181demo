//
// Created by liu on 2021/9/22.
//

#ifndef LIVEPLAYER_ARLIVEENGINEEVENT_H
#define LIVEPLAYER_ARLIVEENGINEEVENT_H

#include "IArLive2Engine.h"
#include <webrtc/modules/utility/include/helpers_android.h>

class LiveEngineEvent : public AR::IArLive2EngineObserver{

public:
    LiveEngineEvent(jobject event);
    ~LiveEngineEvent(void);
};

#endif //LIVEPLAYER_ARLIVEENGINEEVENT_H

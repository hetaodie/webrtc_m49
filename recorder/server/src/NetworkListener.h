#ifndef __CC_NETWORK_LISTENER_H__
#define __CC_NETWORK_LISTENER_H__

#include "message/Message.h"

class NetworkListener{

public:
	virtual void onRecvMessage(Message *message)=0;
};

#endif


#ifndef __CC_NETWORK_LISTENER_IMPL_H__
#define __CC_NETWORK_LISTENER_IMPL_H__

#include "NetworkListener.h"
#include "message/Message.h"

class NetworkListenerImpl : public NetworkListener{

public:
	virtual void onRecvMessage(Message *message){
		printf("NetworkListenerImpl::onRecvMessage");
	}


};



#endif


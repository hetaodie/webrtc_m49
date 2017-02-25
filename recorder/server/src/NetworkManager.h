#ifndef __NETWORK_MANAGER_H__
#define  __NETWORK_MANAGER_H__

#include <stdio.h>
#include <string.h>
#include <list>
#include <map>
#include <thread>

#include "NetworkListener.h"
#include "NetworkListenerImpl.h"
#include "message/Message.h"

using namespace std;


class NetworkManager{

	private:
		static NetworkManager* instance;

		list<NetworkListener *> listeners;

	public:
		NetworkManager()
		{
		}
		
                ~NetworkManager()
		{
			list<NetworkListener *> ::iterator it;

			for (it = listeners.begin(); it != listeners.end(); it ++) {
				delete *it;
			}

			listeners.clear();
		}

		static NetworkManager* getInstance();

		//process data from network
		void handleData(char *data,long unsigned int size);
		void processData(char *buf, long unsigned int size);

		void registerListener(NetworkListener* listener);
		void unregisterListener(NetworkListener* listener);

};

#endif



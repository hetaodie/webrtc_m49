#include "NetworkManager.h"

NetworkManager * NetworkManager::instance = NULL;


NetworkManager * NetworkManager::getInstance()
{
	if(NULL==instance){
		instance = new NetworkManager();
	}
	
	return instance;
}


void NetworkManager::handleData(char *buf, long unsigned int size)
{
	char * p = (char *)malloc(size*sizeof(char));	
	memcpy(p,buf,size);

	std::thread *_subThreadInstance = new std::thread(&NetworkManager::processData, this,p,size);
	_subThreadInstance->detach();		
}


void NetworkManager::processData(char *buf, long unsigned int size){

	if(NULL != buf)
		printf("proccess data in new thread %s",buf);

	Message *message = new Message();

	message->decodeData(buf,size);

	list<NetworkListener *> ::iterator it;

	for (it = listeners.begin(); it != listeners.end(); it ++) {
		(*it)->onRecvMessage(message);
	}

	delete message;
}

void NetworkManager::registerListener(NetworkListener* listener){
	listeners.push_back(listener);
}

void NetworkManager::unregisterListener(NetworkListener* listener){
	list<NetworkListener *> ::iterator it;

	for (it = listeners.begin(); it != listeners.end(); it ++) {
		if(listener == *it){
			listeners.erase(it);
			break;
		}
	}
         
}



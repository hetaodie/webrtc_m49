#ifndef __CC_MESSAGE_H__
#define __CC_MESSAGE_H__ 

#include "string"

using namespace std;

class Message{

private:
	string roomId;
	bool isFrom;	
	char *data;

public:
	Message(){

	}
	
	~Message(){
		if(NULL != data)
			free(data);
	}

	void decodeData(char *p, long unsigned int size);

};




#endif


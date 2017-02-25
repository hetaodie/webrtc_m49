#include "Message.h"
#include "InputSerialize.h"

void Message::decodeData(char *p, long unsigned int size){
	int index = 0;
	roomId = InputSerialize::convertBin32ToStr(p,index);	
	int miscValue = InputSerialize::convertBinToInt8(p,index);	

	if(miscValue != 0)
		isFrom = true;
	else
		isFrom = false;

	int dataSize = InputSerialize::convertBinToInt32(p,index);	
	data = new char[dataSize];
	memcpy(data,p+index,dataSize);	


	printf("room id: %s data:%s %d",roomId.c_str(),data,dataSize);
}



#ifndef InputSerialize_hpp
#define InputSerialize_hpp

#include <stdio.h>
#include <sstream>
#include <string.h>
#include <list>
#include <map>
#include <thread>

using namespace std;


class InputSerialize {
    
    //二进制转int8
    public:  static int convertBinToInt8(char *p,int &index);
    
    //二进制转int16
    public:  static int convertBinToInt16(char *p,int &index);
    
    //二进制转int32
    public:  static int convertBinToInt32(char *p,int &index);
    
    //二进制8转Str
    public:  static char * convertBin8ToStr(char *p,int &index);
    
    //二进制16转Str
    public:  static char * convertBin16ToStr(char *p,int &index);
    
     //二进制32转Str
    public: static  char * convertBin32ToStr(char *p,int &index );
    
};
#endif /* InputSerialize_hpp */

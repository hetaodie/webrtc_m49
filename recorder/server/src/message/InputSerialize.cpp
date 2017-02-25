//
//  InputSerialize.cpp
//  MyCppGame
//
//  Created by apple on 16/10/31.
//

#include "InputSerialize.h"

int InputSerialize::convertBinToInt8(char *p,int &index) {
    int value = 0;
    value += p[index] & 0xff;
    index += 1;
    return value;
}

int InputSerialize::convertBinToInt16(char *p,int &index) {
    //char  p[];
    int value = 0;
    value += (p[index] & 0xff);
    value += (p[index + 1] & 0xff)<<8;
    index += 2;
    return value;
}


int InputSerialize::convertBinToInt32(char *p,int &index) {
    //char  p[];
    int value = 0;
    value += (p[index] & 0xff);
    value += (p[index + 1] & 0xff)<<8;
    value += (p[index + 2] & 0xff)<<16;
    value += (p[index + 3] & 0xff)<<24;
    index += 4;
    return value;
}


char * InputSerialize::convertBin8ToStr(char *p,int &index ) {
    
    int len = convertBinToInt8(p, index);
    
    char * result = new char[len + 1];
    
    for(int i=0;i<len;i++){
        result[i] = p[index+i];
    }

    result[len] = '\0';
    index += len;
    return result;
}


char * InputSerialize::convertBin16ToStr(char *p,int &index ) {
    
    int len = convertBinToInt16(p, index);
    
    char * result = new char[len + 1];
    
    for(int i=0;i<len;i++){
        result[i] = p[index+i];
    }

    result[len] = '\0';
    index += len;
    return result;
}

char * InputSerialize::convertBin32ToStr(char *p,int &index ) {
    
    int len = convertBinToInt32(p, index);
    
    char * result = new char[len + 1];
    
    for(int i=0;i<len;i++){
        result[i] = p[index+i];
    }

    result[len] = '\0';
    index += len;
    return result;
}


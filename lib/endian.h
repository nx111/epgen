#ifndef __ENDIAN_H__
#define __ENDIAN_H__
#include "type.h"

int is_little_endian();
void endian_swap(char ch[], int count);
unsigned char getINT8(unsigned char  value,int endian=X_ENDIAN);
int getINT16(short value,int endian=X_ENDIAN);
int getINT32(int value,int endian=X_ENDIAN);
int getINT64(long value,int endian=X_ENDIAN);

#endif

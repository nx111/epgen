#include "endian.h"

extern int running_endian;
extern int data_endian;

int is_little_endian()
{
	int one = 1;
	return *(char*)&one;
}

void endian_swap(char ch[], int count)
{
	int size = count / 2;
	int max = count - 1;
	for (int i = 0; i < size; i++) {
		char t = ch[i];
		ch[i] = ch[max - i];
		ch[max - i] = t;
	}
}

unsigned char getINT8(unsigned char  value,int endian)
{
	unsigned char a=value & 0x0F;
	unsigned char b=value & 0xF0;
	unsigned char ret=value;
	
	int Endian=endian;
	if(Endian==X_ENDIAN)
		Endian=data_endian;
	if(Endian!=running_endian && Endian!=X_ENDIAN)
		ret=(a<<4 | b>>4);
	return ret;
}

int getINT16(short value,int endian)
{
	union{
		short _i;
		char _c[sizeof(short)];
	}un;
	un._i=value;
	
	int Endian=endian;
	if(Endian==X_ENDIAN)
		Endian=data_endian;
	if(Endian!=running_endian && Endian!=X_ENDIAN)
		endian_swap(un._c,sizeof(short));
	return un._i;
}

int getINT32(int value,int endian)
{
	union{
		int _i;
		char _c[sizeof(int)];
	}un;
	un._i=value;

	int Endian=endian;
	if(Endian==X_ENDIAN)
		Endian=data_endian;
	if(Endian!=running_endian && Endian!=X_ENDIAN)
		endian_swap(un._c,sizeof(int));
	return un._i;
}


int getINT64(long value,int endian)
{
	union{
		long _i;
		char _c[sizeof(int)];
	}un;
	un._i=value;

	int Endian=endian;
	if(Endian==X_ENDIAN)
		Endian=data_endian;
	if(Endian!=running_endian && Endian!=X_ENDIAN)
		endian_swap(un._c,sizeof(long));
	return un._i;
}


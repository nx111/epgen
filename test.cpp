#include <stdio.h>
#include "lib/estring.h"

int main(){
	FILE *f=fopen("test_in.txt","r");
	if(!f){
		puts("No test_in.txt,exit!\n");
		return -1;
	}
	unsigned char temp[2048];
	int rlen=fread(temp,1,2048,f);
	eString outs=GB18030ToUTF8(temp,rlen);
	printf("OUTPUT:%s\n",outs.c_str());
	return 0;
}


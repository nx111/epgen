#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include "gb18030.h"
#include "big5.h"
#include "estring.h"

extern int littleEndian,running_endian;

///////////////////////////////////////// eString sprintf /////////////////////////////////////////////////
eString& eString::sprintf(char *fmt, ...)
{
// Implements the normal sprintf method, to use format strings with eString
// The max length of the result string is 1024 char.
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString setNum(uint, uint) ///////////////////////////////////////
eString& eString::setNum(int val, int sys)
{
//	Returns a string that contain the value val as string
//	if sys == 16 than hexadezimal if sys == 10 than decimal
	char buf[12];

	if (sys == 10)
		snprintf(buf, 12, "%i", val);
	else if (sys == 16)
		snprintf(buf, 12, "%X", val);		
	
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString replaceChars(char, char) /////////////////////////////
eString& eString::removeChars(char fchar)
{
//	Remove all chars that equal to fchar, and returns a reference to itself
	unsigned int index=0;

	while ( ( index = find(fchar, index) ) != npos )
		erase(index, 1);

	return *this;
}

/////////////////////////////////////// eString upper() ////////////////////////////////////////////////
eString& eString::upper()
{
//	convert all lowercase characters to uppercase, and returns a reference to itself
	for (iterator it = begin(); it != end(); it++)
		switch(*it)
		{
			case 'a' ... 'z' :
				*it -= 32;
			break;

		}
	return *this;
}

eString& eString::trim()
{
  iterator i;
    for (i = begin(); i != end(); i++) {
        if ( (unsigned char)(*i) > 0x20 ) {
            erase(begin(), i);
            break;
        }
    }

    if (i == end()) {
        return *this;
    }

    for (i = end() - 1; i != begin(); i--) {
        if ( (unsigned char)(*i) > 0x20) {
            erase(i + 1, end());
            break;
        }
    }

    return *this;
}

eString& eString::strReplace(const char* fstr, const eString& rstr,int encode)
{
//	replace all occurrence of fstr with rstr and, and returns a reference to itself
	unsigned int index=0;
	unsigned int fstrlen = strlen(fstr);
	int rstrlen=rstr.size();

	switch(encode){
	case UTF8_ENCODING:
		while(index<length()){
			if( (fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr) ){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			if((at(index) & 0xE0)==0xC0)
				index+=2;
			else
			if((at(index) & 0xF0)==0xE0)
				index+=3;
			else
			if((at(index) & 0xF8)==0xF0)
				index+=4;
			else
				index++;
		}
		break;
	case BIG5_ENCODING:
	case GB18030_ENCODING:
		while(index<length()){
			if((fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr)){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			if((index+1)>=length())break;
			unsigned char c1=at(index);
			unsigned char c2=at(index+1);
			if ( (c1>0x80 && c1<0xff && c2>=0x40 && c2<0xff)	//GBK
				||(c1>0xa0 && c1<0xf9 && ((c2>=0x40 && c2<0x7f)||(c2>0xa0 && c2<0xff)))	//BIG5
			)
				index+=2;
			else
				index++;
		}
		break;
	case UNICODE_ENCODING:
		while(index<length()){
			if((fstrlen+index)<=length() && !strcmp(mid(index,fstrlen).c_str(),fstr)){
				replace(index,fstrlen,rstr);
				index+=rstrlen;
				continue;
			}
			index+=2;
		}
		break;

	default:
		while ( ( index = find(fstr, index) ) != npos )
		{
			replace(index, fstrlen, rstr);
			index+=rstrlen;
		}
		break;
	}
	return *this;
}

/////////////////////////////////////// eString icompare(const  eString&) /////////////////////
int eString::icompare(const eString& s)
{
//	makes a case insensitive string compare
	std::string::const_iterator p = begin(), p2 = s.begin();

	while ( p != end() && p2 != s.end() )
		if ( tolower(*p) != tolower(*p2) )
			return tolower(*p) < tolower(*p2) ? -1 : 1;
		else
			p++, p2++;

	return length() == s.length() ? 0 : length() < s.length() ? -1 : 1;
}

eString eString::mid(unsigned int index, unsigned int len,int encode) const
{
	unsigned int i=0,j=0;
	switch(encode){
	case UTF8_ENCODING:
		while(i<length()){
			unsigned int nextpos=stepUTF8(*this,i);
			if(i<=index && nextpos>index)
				j=i;
			i=nextpos;
			if(i>index && (i-j)>len)
				break; 
		}
		break;
	case BIG5_ENCODING:
	case GB18030_ENCODING:
		while(i<length()){
			if((i+2)>=length())break;
			unsigned char c1=at(i);
			unsigned char c2=at(i+1);
			unsigned int nextpos=i;
			if ( (c1>0x80 && c1<0xff && c2>=0x40 && c2<0xff)	//GBK
				||(c1>0xa0 && c1<0xf9 && ((c2>=0x40 && c2<0x7f)||(c2>0xa0 && c2<0xff)))	//BIG5
			)
				nextpos+=2;
			else
				nextpos++;

			if(i<=index && nextpos>index)
				j=i;
			i=nextpos;
			if(i>index && (i-j)>len)
				break; 
			
		}
		break;
	case UNICODE_ENCODING:
		while(i<length()){
			if((i+2)>=length())break;
			if(i<=index && (i+2)>index)
				j=i;
			i+=2;
			if(i>index && (i-j)>len)
				break; 
		}
		break;

	default:
		while(i<length()){
			if((i+1)>=length())break;
			if(i<=index && (i+1)>index)
				j=i;
			i+=1;
			if(i>index && (i-j)>len)
				break; 
		}
		break;

	}
	
	return mid(j,i-j);	
}

int UnicodeToUTF8(long c, char *out)
{
	char *s = out;
	int ret = 0;

	if (c < 0x80){
		*(s++) = c;
		ret = 1;
	}else if (c < 0x800)
	{
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 2;
	}
	else if (c < 0x10000)
	{
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 3;
	}
	else if (c < 0x200000)
	{
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
		ret = 4;
	}
	return ret;
}


eString GB18030ToUTF8(const unsigned char *szIn, int len,int *pconvertedLen)
{
	char szOut[len * 2];
	unsigned long code=0;
	int t=0,i=0;
	for(i=0; i < (len-1);){
		int cl=0,k=0;

		cl=gb18030_mbtowc((ucs4_t*)(&code),(const unsigned char *)szIn+i,len-i);
		if(cl>0)
			k=UnicodeToUTF8(code,szOut+t);
		t+=k;
		if(cl>0)
			i+=cl;
		else
			i++;
	}
	szOut[t]='\0';
	if(pconvertedLen)*pconvertedLen=i;
	return eString(szOut,t);
}

eString UTF8ToGB2312(const unsigned char *szIn,int slen)
{
	unsigned long code=0;
	unsigned char temp[4096];
	unsigned int j=0;
	int len=slen;
	if(!slen)
		len=strlen((const char*)szIn);

	for (int i=0; i < len; ++i)
	{
		if (!(szIn[i]&0x80)){ // normal ASCII
			temp[j++]=szIn[i];
			continue;
		}else if ((szIn[i] & 0xE0) == 0xC0) // one char following.
		{
				// first, length check:
			if (i+1 >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=(szIn[i] & 0x1F)<<6 | (szIn[i+1] & 0x3F);
			i++;	
		} else if ((szIn[i] & 0xF0) == 0xE0)
		{
			if ((i+2) >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 ){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x0F)<<12) | ((szIn[i+1] & 0x3F)<<6) | (szIn[i+2] & 0x3F);
			i+=2;
		} else if ((szIn[i] & 0xF8) == 0xF0)
		{
			if ((i+3) >= len){
				temp[j++]=szIn[i];
				break;
			}else	if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;
			}
			code=((szIn[i] & 0x07)<<18)|((szIn[i+1] & 0x3F)<<12) | ((szIn[i+2] & 0x3F)<<6) | (szIn[i+3] & 0x3F);
			i+=3;
		} else if ((szIn[i] & 0xFC) == 0xF8)
		{
			if ((i+4) >= len){
				temp[j++]=szIn[i];
				break;
			}else if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80
				|| (szIn[i+4]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];		
				continue;
			}
			code=((szIn[i] & 0x03)<<24)|((szIn[i+1] & 0x3F)<<18)|((szIn[i+2] & 0x3F)<<12) | ((szIn[i+3] & 0x3F)<<6) | (szIn[i+4] & 0x3F);
			i+=4;
		} else if ((szIn[i] & 0xFD) == 0xFC)
		{
			if ((i+5) >= len){
				temp[j++]=szIn[i];
				break;
			}else if ((szIn[i+1]&0xC0) != 0x80 || (szIn[i+2]&0xC0) != 0x80 || (szIn[i+3]&0xC0) != 0x80
				|| (szIn[i+4]&0xC0) != 0x80 || (szIn[i+5]&0xC0) != 0x80){
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i++];
				temp[j++]=szIn[i];
				continue;		
			}
			code=((szIn[i] & 0x01)<<30)|((szIn[i+1] & 0x03F)<<24)|((szIn[i+2] & 0x3F)<<18)|((szIn[i+3] & 0x3F)<<12) | ((szIn[i+4] & 0x3F)<<6) | (szIn[i+5] & 0x3F);
		i+=5;
		}else{
			temp[j++]=szIn[i];
			continue;
		}
		unsigned char mb[3];		
		if(2!=gb2312_wctomb(mb,code,2))
			continue;
		temp[j++]=mb[0] | 0x80;
		temp[j++]=mb[1] | 0x80;
	}
	temp[j]='\0';
	return eString((const char *)temp,j);
}
eString Big5ToUTF8(const unsigned char *szIn, int len,int *pconvertedLen)
{
	char szOut[len * 2];
	unsigned long code=0;
	int t=0,i=0;

	for(;i<(len-1);i++){
		if((szIn[i]>0xA0) && szIn[i]<=0xF9 &&(
			((szIn[i+1]>=0x40)&&(szIn[i+1]<=0x7F)) || ((szIn[i+1]>0xA0)&&(szIn[i+1]<0xFF))
		    )){
			big5_mbtowc((ucs4_t*)(&code),(const unsigned char *)szIn+i,2);
			int k=UnicodeToUTF8(code,szOut+t);
			t+=k;
			i++;
		     }
		else 
			szOut[t++]=szIn[i];
	}

  	if(i<len && szIn[i] && (szIn[i]<0xA0 || szIn[i]>0xF9))
		szOut[t++]=szIn[i++];

	if(pconvertedLen)*pconvertedLen=i;
	return eString(szOut,t);
	
}

eString convertDVBUTF8(const unsigned char *data, int len, int table, int tsidonid,int noEncodeID,int *pconvertedLen)
{
//	printf("[convertDVBUTF]len=%d\ttable=%02x\tdata[0]=0x%02x data=%s\n",len,table,data[0],data);
	if (!len){
		if(pconvertedLen)*pconvertedLen=0;
		return "";
	}

	int i=0, t=0;
	int encode=table;
	

//	eDebug("ConvertDVBUTF8-1:<data=%s><table=0x%x><tsidonid=%d>\n",data,table,tsidonid);
	if (encode!=UNICODE_ENCODING && encode!=UTF16BE_ENCODING && encode!=UTF16LE_ENCODING ) 
	 switch(data[0])
	 {
		case 1 ... 11:
			encode=data[i++]+4;
//			eDebug("(1..12)text encoded in ISO-8859-%d",table);
			break;
		case 0x10:
		{
//			eDebug("(0x10)text encoded in ISO-8859-%d",n);
			int n=(data[i+1]<<8)|(data[i]);
			i+=2;
			switch(n)
			{
				case 12: 
				default:
					encode=n;
					break;
			}
			break;
		}
		case 0x11: //  Basic Multilingual Plane of ISO/IEC 10646-1 enc  (UTF-16... Unicode)
			encode=UTF16BE_ENCODING;
			++i;
			break;
		case 0x12:
			++i;
			break;
		case 0x13:
			++i;
			encode=GB18030_ENCODING;
			break;
		case 0x14:
			++i;
			encode=BIG5_ENCODING;
			break;
		case 0x15: // UTF-8 encoding of ISO/IEC 10646-1
			encode=UTF8_ENCODING;
			return std::string((char*)data+1, len-1);
			break;
		case 0x16:
			encode=UNICODE_ENCODING;
			++i;
			break;
		case 0x17:
			encode=UTF16BE_ENCODING;
			++i;
			break;
		case 0x18:
			encode=UTF16LE_ENCODING;
			++i;
			break;
		case 0:
		case 0xD ... 0xF:
		case 0x19 ... 0x1F:
			++i;
			break;
	}

	unsigned char res[4096];
	switch(encode)
	{
		case 0 ... 16:
		case 0x12:
		case 0x15 ... 0x1F:
		{
 		  while (i < len)
		  {
			unsigned long code=0;

	
			if (encode==UNICODE_ENCODING){
				unsigned long code1=data[i];
				unsigned long code2=data[i+1];
				code=(code2<<8)|code1;
				i++;
			}
			else if(encode==UTF16BE_ENCODING){
				if((i+2)>len)break;
				unsigned long w1=((unsigned long)(data[i])<<8) |((unsigned long)(data[i+1]));
				if(w1<0xD800UL || w1>0xDFFFUL){
					code=w1;
					i++;
				}
				else if(w1>0xDBFFUL)
					break;
				else if((i+4)<len){
					unsigned long w2=((unsigned long)(data[i+2])<<8) |((unsigned long)(data[i+3]));
					if(w2<0xDC00UL || w2>0xDFFFUL)return eString("");
					code=0x10000UL + (((w1 & 0x03FFUL)<<10 ) | (w2 & 0x03FFUL));
					i+=3;
				}
				else 
					break;
			}
			else if(encode==UTF16LE_ENCODING){
				if((i+2)>len)break;
				unsigned long w1=((unsigned long)(data[i+1])<<8) |((unsigned long)(data[i]));
				if(w1<0xD800UL || w1>0xDFFFUL){
					code=w1;
					i++;
				}
				else if(w1>0xDBFFUL)
					break;
				else if((i+4)<len){
					unsigned long w2=((unsigned long)(data[i+3])<<8) |((unsigned long)(data[i+2]));
					if(w2<0xDC00UL || w2>0xDFFFUL)break;
					code=0x10000UL + (((w1 & 0x03FFUL)<<10 ) | (w2 & 0x03FFUL));
					i+=3;
				}
				else
					break;
			}
			else if  (encode==0 || encode==UTF8_ENCODING){ 	// UTF-8 or default
				res[t++]=data[i++];
				continue;
			}

			if (!code)
				continue;
	
			if (code < 0x80){ // identity ascii <-> utf8 mapping
				res[t++]=char(code);
				}
			else if (code < 0x800) // two byte mapping
			{
				res[t++]=(code>>6)|0xC0;
				res[t++]=(code&0x3F)|0x80;
			} else if (code < 0x10000) // three bytes mapping
			{
				res[t++]=(code>>12)|0xE0;
				res[t++]=((code>>6)&0x3F)|0x80;
				res[t++]=(code&0x3F)|0x80;
			} else
				{
				res[t++]=(code>>18)|0xF0;
				res[t++]=((code>>12)&0x3F)|0x80;
				res[t++]=((code>>6)&0x3F)|0x80;
				res[t++]=(code&0x3F)|0x80;
			}
			i++;
			if (t+4 > 4095)	{
				printf("convertDVBUTF8 buffer to small,must not more than 4095 bytes. break now!\n");
				break;
				}
			}
        
		  if(pconvertedLen)*pconvertedLen=i;
		  return eString((char*)res, t);
		  break;
        	}
		case GB18030_ENCODING:
			return GB18030ToUTF8((const unsigned char *)(data + i), len - i,pconvertedLen);
			break;
		case BIG5_ENCODING:
			return Big5ToUTF8((const unsigned char *)(data + i), len - i,pconvertedLen);
	}
	return eString("");
	
}


int isUTF8(const eString &string)
{
	unsigned int len=string.size();
	
	for (unsigned int i=0; i < len; ++i)
	{
		if (!(string[i]&0x80)) // normal ASCII
			continue;
		if ((string[i] & 0xE0) == 0xC0) // one char following.
		{
				// first, length check:
			if (i+1 >= len)
				return 0; // certainly NOT utf-8
			if ((string[i+1]&0xC0) != 0x80)
				return 0; // no, not UTF-8.
			i++;
		} else if ((string[i] & 0xF0) == 0xE0)
		{
			if ((i+2) >= len)
				return 0;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 )
				return 0;
			i+=2;
		} else if ((string[i] & 0xF8) == 0xF0)
		{
			if ((i+3) >= len)
				return 0;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80)
				return 0;
			i+=3;
		} else if ((string[i] & 0xFC) == 0xF8)
		{
			if ((i+4) >= len)
				return 0;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80
				|| (string[i+4]&0xC0) != 0x80)
				return 0;
			i+=4;
		} else if ((string[i] & 0xFD) == 0xFC)
		{
			if ((i+5) >= len)
				return 0;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80
				|| (string[i+4]&0xC0) != 0x80 || (string[i+5]&0xC0) != 0x80)
				return 0;
			i+=5;
		}

	}
	return 1; // can be UTF8 (or pure ASCII, at least no non-UTF-8 8bit characters)
}

int stepUTF8(const eString &string,int pos)
{
	unsigned int len=string.size();
	unsigned int i;
	for (i=pos; i < len; ++i)
	{
		if (!(string[i]&0x80)) // normal ASCII
			break;
		if ((string[i] & 0xE0) == 0xC0) // one char following.
		{
				// first, length check:
			if (i+1 >= len)
				break; // certainly NOT utf-8
			if ((string[i+1]&0xC0) != 0x80)
				break; // no, not UTF-8.
			i++;
		} else if ((string[i] & 0xF0) == 0xE0)
		{
			if ((i+2) >= len)
				break;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 )
				break;
			i+=2;
		} else if ((string[i] & 0xF8) == 0xF0)
		{
			if ((i+3) >= len)
				break;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80)
				break;
			i+=3;
		} else if ((string[i] & 0xFC) == 0xF8)
		{
			if ((i+4) >= len)
				break;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80
				|| (string[i+4]&0xC0) != 0x80)
				break;
			i+=4;
		} else if ((string[i] & 0xFD) == 0xFC)
		{
			if ((i+5) >= len)
				break;
			if ((string[i+1]&0xC0) != 0x80 || (string[i+2]&0xC0) != 0x80 || (string[i+3]&0xC0) != 0x80
				|| (string[i+4]&0xC0) != 0x80 || (string[i+5]&0xC0) != 0x80)
				break;
			i+=5;
		}
		break;

	}
	return i+1; 
}

int isSpaceChar(char ch)
{
	unsigned char chin=(unsigned char)ch;
	return (chin<=' ');
}

eString XML_ENCODE(eString &sin)
{
	eString tmp;
	tmp=sin.strReplace("&","&amp;");
	tmp=tmp.strReplace("<","&lt;");
	tmp=tmp.strReplace(">","&gt;");
//	tmp=tmp.strReplace(" ","&nbsp;");
	return tmp;	
}

eString XML_DECODE(eString &sin)
{
	eString tmp;
	tmp=sin.strReplace("&amp;","&");
	tmp=tmp.strReplace("&lt;","<");
	tmp=tmp.strReplace("&gt;",">");
	tmp=tmp.strReplace("&nbsp;"," ");
	return tmp;	
}

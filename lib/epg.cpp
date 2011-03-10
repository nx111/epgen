//#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#ifdef __WIN32__
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

#include "type.h"
#include "endian.h"
#include "dvb.h"
#include "epg.h"
#include "estring.h"
#include "libmd5sum/libmd5sum.h"

#include "spxml/spdomparser.hpp"
#include "spxml/spdomiterator.hpp"
#include "spxml/spxmlnode.hpp"

#define HILO(x) (x##_hi << 8 | x##_lo)

#define tempsize 8196

int data_endian=X_ENDIAN;
int running_endian=X_ENDIAN;	//1: big endian     2: little endian

eventCache eventDB;
tsonidMap tvmap;
tvNameMap tvnmap;
tvAllNameMap tvamap;
long timeoff;


#ifdef __WIN32__
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon     = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min     = wtm.wMinute;
    tm.tm_sec     = wtm.wSecond;
    tm. tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}

void init()
{
	running_endian=is_little_endian()+1;
	TIME_ZONE_INFORMATION TimeZoneInfo;
	GetTimeZoneInformation( &TimeZoneInfo );
	timeoff=TimeZoneInfo.Bias * 60;
}

#else
void init()
{
	running_endian=is_little_endian()+1;
	struct timeval  tv;
	struct timezone tz;

	gettimeofday(&tv,&tz);
	timeoff=tz.tz_minuteswest * 60;
//	printf("timeoff:%ld\n",timeoff);

}

#endif

time_t mktimet_from_xmltv(const char *stime)
{
	tm t;
	char year[5],mon[3],day[3],hour[3],minu[3],sec[3];

	strncpy(year,stime,4);
	strncpy(mon,stime+4,2);
	strncpy(day,stime+6,2);
	strncpy(hour,stime+8,2);
	strncpy(minu,stime+10,2);
	strncpy(sec,stime+12,2);
	year[4]=0;mon[2]=0;day[2]=0;hour[2]=0;minu[2]=0;sec[2]=0;
	t.tm_year=atoi(year)-1900;
	t.tm_mon=atoi(mon)-1;
	t.tm_mday=atoi(day);
	t.tm_hour=atoi(hour);
	t.tm_min=atoi(minu);
	t.tm_sec=atoi(sec);
	t.tm_isdst =  -1;

	return mktime(&t);
}
string strtime(time_t time)
{
	struct tm *ptr;
	char str[80];
	time_t t=time-timeoff;
	ptr=localtime(&t);
	strftime(str,80,"%Y-%m-%d %H:%M:%S",ptr);
	return str;
}

epg::epg():endian(B_ENDIAN),maxEventID(0),debug(0)
{
	tvmap.clear();
	tvnmap.clear();
	tvamap.clear();
}

int epg::loadepg(eString epgfile,int filetype)
{
	if(filetype)
		return 1;
	if(epgfile.find(".xml")!=string::npos || epgfile.find(".XML")!=string::npos)
	{
		data_endian=B_ENDIAN;
		return loadepg_from_xmltv(epgfile);
	}

	FILE *f = fopen(epgfile.c_str(), "rb");
	if (f)
	{
		unsigned int magic=0;
		fread( &magic, sizeof(int), 1, f);
		if (!memcmp((__u8*)(&magic),"\x98\x76\x54\x32",4))
			endian=B_ENDIAN;
		else if(!memcmp((__u8*)(&magic),"\x32\x54\x76\x98",4))
			endian=L_ENDIAN;
		else
		{
			puts("Not a valid epg.dat file!");
			fclose(f);
			return -1;
		}
		data_endian=endian;
		
		char text1[13];
		fread( text1, 13, 1, f);
		if ( !strncmp( text1, "ENIGMA_PLI_V5", 13) )
		{
			int size=0;
			int cnt=0;

			int source=(endian==B_ENDIAN)?srPLI_EPGDAT_BE:srPLI_EPGDAT_LE;
			fread( &size, sizeof(int), 1, f);
			size=getINT32(size);
			while(size--)
			{
				uniqueEPGKey key0,key;
				timeMap  tmMap;
				eventMap evMap;
				int size=0;
				if(!fread( &key0, sizeof(uniqueEPGKey), 1, f))break;
				key.sid=getINT32(key0.sid);
				key.onid=getINT32(key0.onid);
				key.tsid=getINT32(key0.tsid);
				tsonidMap::iterator ikey=tvmap.find(key);
				if(ikey != tvmap.end())
					key=ikey->second;

				fread( &size, sizeof(int), 1, f);
				size=getINT32(size);
//				printf("\tsize=%x\n",size);
				while(size--)
				{
					__u16 len1=0,len=0;
					__u8 type=0;
					eventData *event=0;
//					printf("%d tsonid=%d:%d:%d offset1=%x",cnt,key.sid,key.onid,key.tsid,ftell(f));
					fread( &type, sizeof(__u8), 1, f);
					fread( &len1, sizeof(__u16), 1, f);
					len=getINT16(len1);
					event = new eventData(0, len, type,source);
					event->EITdata = new __u8[len];
					fread( event->EITdata, 1, len, f);
//					printf("  len=%x   offset3=%x\n",len,ftell(f));
					adjust_eit_event_endian(event->EITdata,B_ENDIAN);
					eventData::CacheSize+=len;
					evMap[ event->getEventID() ]=event;
					tmMap[ event->getStartTime() ]=event;
					if(maxEventID < event->getEventID())
						maxEventID=event->getEventID();
					++cnt;
				}
				eventDB[key]=pair<eventMap,timeMap>(evMap,tmMap);
			}
			eventData::load(f,source);
			printf("[EPGM] %d events read from ENIGMA_PLI_V5 %s\n", cnt, epgfile.c_str());
		}
		else if ( !strncmp( text1, "ENIGMA_EPG_V7", 13) )
		{
		//add support enigma_epg_v7 epg.dat here
			int size=0;
			int cnt=0;

			int source=(endian==B_ENDIAN)?srGEMINI_EPGDAT_BE:srGEMINI_EPGDAT_LE;

			fread( &size, sizeof(int), 1, f);
			size=getINT32(size);
//			printf("channels:%d\n",size);
			while(size--)
			{
				uniqueEPGKey key0,key;
				timeMap tmMap;
				eventMap evMap;
				int size=0;
				fread( &key0, sizeof(uniqueEPGKey), 1, f);
				key.sid=getINT32(key0.sid);
				key.onid=getINT32(key0.onid);
				key.tsid=getINT32(key0.tsid);

				tsonidMap::iterator ikey=tvmap.find(key);
				if(ikey != tvmap.end())
					key=ikey->second;
				fread( &size, sizeof(int), 1, f);
				size=getINT32(size);

				while(size--)
				{
					__u8 len=0;
					__u8 type=0;
					eventData *event=0;
					fread( &type, sizeof(__u8), 1, f);
					fread( &len, sizeof(__u8), 1, f);
					event = new eventData(0, len, type,source);
					event->EITdata = new __u8[len];
					fread( event->EITdata, 1, len, f);
//					if (debug)
//						debug_eit((eit_event_struct*)event->EITdata);
					eventData::CacheSize+=len;
					tmMap[ event->getStartTime() ]=event;
					evMap[ event->getEventID() ]=event;
					if(maxEventID < event->getEventID())
						maxEventID=event->getEventID();
					++cnt;
				}
				eventDB[key]=pair<eventMap,timeMap>(evMap,tmMap);
			}
			eventData::load(f,source);
			printf("[EPGM] %d events read from ENIGMA_EPG_V7 %s\n", cnt, epgfile.c_str());
		}
		else
		{
			printf("[EPGM] don't read old epg database\n");
		}
		fclose(f);
	}
	return 0;
}

int epg::saveepg(eString epgfile,int targetMode,int bomMode)
{
	if(epgfile.find(".xml")!=string::npos || epgfile.find(".XML")!=string::npos)
		return saveepg_to_xmltv(epgfile,bomMode);
	
	FILE *f = fopen(epgfile.c_str(), "wb");
	int cnt=0;
	if ( f )
	{
		unsigned int magic = 0x98765432;
		int endian=B_ENDIAN;
		if(targetMode==srPLI_EPGDAT_LE || targetMode==srGEMINI_EPGDAT_LE)
			endian=L_ENDIAN;
		magic=getINT32(magic,endian);
		fwrite( &magic, sizeof(int), 1, f);
		string text;
		if(targetMode==srPLI_EPGDAT_LE || targetMode==srPLI_EPGDAT_BE) 
			text="ENIGMA_PLI_V5";
		else text="ENIGMA_EPG_V7";
		fwrite( text.c_str(), 13, 1, f );
		int size = getINT32(eventDB.size(),endian);
		fwrite( &size, sizeof(int), 1, f );
		for (eventCache::iterator service_it(eventDB.begin()); service_it != eventDB.end(); ++service_it)
		{
			timeMap &tmMap = service_it->second.second;
			uniqueEPGKey key;
			key.tsid=getINT32(service_it->first.tsid,endian);
			key.onid=getINT32(service_it->first.onid,endian);
			key.sid=getINT32(service_it->first.sid,endian);
			fwrite((__u8*)&key, sizeof(uniqueEPGKey), 1, f);
			size = getINT32(tmMap.size(),endian);
			fwrite((__u8*)&size, sizeof(int), 1, f);
			for (timeMap::iterator ev_it(tmMap.begin()); ev_it != tmMap.end(); ++ev_it)
			{
				if((targetMode==srPLI_EPGDAT_LE || targetMode==srPLI_EPGDAT_BE) ){
					eit_event_struct *eit;
					eit=ev_it->second->get_v5(endian);
					__u16 len = HILO(eit->descriptors_loop_length)+12;
					__u16 wlen=getINT16(len,endian);
					
					fwrite((__u8*)&(ev_it->second->type), sizeof(__u8), 1, f );
					fwrite((__u8*)(&wlen), sizeof(__u16), 1, f);
					fwrite( (__u8*)eit, len, 1, f);
				}else{
					__u8 *eit;
					int len=0;
					eit=ev_it->second->get_v7(&len,endian);
					__u8 clen=(len & 0xff);
					fwrite((__u8*)&(ev_it->second->type), sizeof(__u8), 1, f );
					fwrite((__u8*)&clen, sizeof(__u8), 1, f);
					fwrite(eit, clen, 1, f);
				}
				++cnt;
			}
		}
		eventData::save(f,targetMode);
		fclose(f);
		f=fopen((epgfile+".md5").c_str(),"wb");
		unsigned char md5[16];
		if(f && !md5_file(epgfile.c_str(), 1, md5)){
			fwrite(md5,1,16,f);
			fclose(f);
		}
	}
	return 0;
}

int epg::loadepg_from_event_struct(event_data_struct &eds)
{
	tsonidMap::iterator ikey=tvmap.find(eds.tsonid);
	if(ikey != tvmap.end()){
		eventCache::iterator ie=eventDB.find(ikey->second);
		if(ie == eventDB.end() ){
			timeMap tmMap;
			eventMap evMap;
			eventDB[ikey->second]=pair<eventMap,timeMap>(evMap,tmMap);
//			return 0;
		}
		eventDB[eds.tsonid]=eventDB[ikey->second];
		
	}else{
		eventCache::iterator ie=eventDB.find(eds.tsonid);
		if(ie == eventDB.end() ){
			timeMap tmMap;
			eventMap evMap;
			eventDB[eds.tsonid]=pair<eventMap,timeMap>(evMap,tmMap);
		}
	}
		

	eString ShortEventName=XML_DECODE(eds.title);
//	eString ShortEventText="";
	eString ExtendEventText=XML_DECODE(eds.desc);

//	printf("[N]%s\n[X]%s\n",ShortEventName.c_str(),ExtendEventText.c_str());

	__u8 EITtmp[256];

	__u16 newid=maxEventID+1;
	
	if(eds.event_id != -1)
		newid=eds.event_id;
//	printf("newid=%04X\n",newid);
	EITtmp[0]=(newid >>8) & 0x0FF;
	EITtmp[1]=newid & 0x0FF;

	makeDVBtime(EITtmp+2,eds.start_time);

	EITtmp[7]=toBCD(eds.duration/3600);
	EITtmp[8]=toBCD((eds.duration % 3600)/60);
	EITtmp[9]=toBCD(eds.duration % 60);


	int ptr=10;
	int optr=0;
	__u32 *p = (__u32*)(EITtmp+10);
	__u8 *descrData=new __u8[ShortEventName.size()+6+2];
	descrData[optr++]='\x4D';
	descrData[optr++]=ShortEventName.size()+6;
	descrData[optr++]='e';
	descrData[optr++]='n';
	descrData[optr++]='g';
	descrData[optr++]=ShortEventName.size()+1;
	descrData[optr++]=eds.encode;
	memcpy(descrData+optr,ShortEventName.c_str(),ShortEventName.size());
	optr+=ShortEventName.size();

	descrData[optr++]=0;	//ShortEventText Length

	__u32 crc = 0;
	int cnt=0;
	optr=0;
	while(cnt++ < (descrData[1]+2))
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ descrData[optr++]) & 0xFF];

	descriptorMap::iterator it = eventData::descriptors.find(crc);
	if ( it == eventData::descriptors_target.end() )
	{
		eventData::descriptors[crc]= descriptorPair(1,descrData);
	}
	else{
		++(it->second.first);
		delete []descrData;
	}
	(*p++)=getINT32(crc,B_ENDIAN);
	ptr+=sizeof(__u32);


	__u8 last_descriptor_number=0,descriptor_number=0;
	for(unsigned int pos=0;pos<ExtendEventText.size();){
	   	if(pos>=ExtendEventText.size())break;
		unsigned int maxlen=240;
		if(!pos){
		 	maxlen = ExtendEventText.find(':', 0);
			if(maxlen == std::string::npos || maxlen>240)
				maxlen=240;
			else
			   maxlen++;
		}
		else 
			maxlen=240;

		eString eDesc=ExtendEventText.mid(pos,maxlen,eds.encode);
		if(0==eDesc.size())break;
		pos+=eDesc.size();

		descrData=new __u8[eDesc.size()+7+2];
		optr=0;
		descrData[optr++]='\x4E';	
		descrData[optr++]=eDesc.size()+7;
		descrData[optr++]=((descriptor_number &0x0F)<<4) | (last_descriptor_number & 0x0F);
		descrData[optr++]='e';
		descrData[optr++]='n';
		descrData[optr++]='g';
		descrData[optr++]=0;	//length_of_items
		descrData[optr++]=eDesc.size()+1;
		descrData[optr++]=eds.encode;
		
		adjust_eit_extented_descriptor_endian(descrData,B_ENDIAN);
		memcpy(descrData+optr,eDesc.c_str(),eDesc.size());

		crc = 0;
		cnt=0;
		optr=0;
		while(cnt++ < (descrData[1]+2))
			crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ descrData[optr++]) & 0xFF];

		it = eventData::descriptors.find(crc);
		if ( it == eventData::descriptors.end() )
		{
			eventData::descriptors[crc]=descriptorPair(1,descrData);
		}
		else{
			++(it->second.first);
			delete []descrData;
		}
		(*p++)=getINT32(crc,B_ENDIAN);
		ptr+=sizeof(__u32);

		last_descriptor_number=descriptor_number;
		descriptor_number++;
		
	}
	eit_event_struct* EITdata=(eit_event_struct* )new __u8[ptr];
	memcpy((__u8*)EITdata,(__u8*)EITtmp,ptr);
	eventData *event = new eventData(EITdata, ptr,SCHEDULE,srGEMINI_EPGDAT_BE);

	std::pair<eventMap,timeMap> &servicemap = eventDB[eds.tsonid];
	servicemap.first[newid]=event;
	servicemap.second[eds.start_time]=event;

	if(maxEventID<newid)
		maxEventID=newid;
	return 0;
}

int epg::load_tvmap(eString mapfile)
{
	FILE *f = fopen(mapfile.c_str(), "rt");
	if(!f)return -1;
	tvnmap.clear();
	tvmap.clear();
	

	char *line = (char*) malloc(256);
	int bufsize=256;
	int count, i;
	int sid,tsid, onid;
	while( fgets(line, bufsize, f) != NULL )
	{
		for(i=0;line[i] && line[i]!='#' ;i++);
		line[i]=0;

		char sec[4][250];
		const char splitch[]=":=";
		char *p=strtok(line,splitch);
		char *search;
		count=0;
		do{
			char *s=sec[count];

			/***********   trim  begin **********/
			for(search=p; isSpaceChar(*search) ; search++);

			do {
				*s++ = *search++;
			}while (*search != '\0');

			search = s;
			for(search--;isSpaceChar(*search); search--);

			search++;
			*search='\0' ;
			/**************  trim end  ************/
			count++;
			p=strtok(NULL,splitch);
		}while(p);

		if(count<4)continue;

		char chname[256];
		int spacech=0;
		for(search=sec[3],i=0;search<(sec[3]+strlen(sec[3])) && i<255;search++){
			if (*search==' ' ||  *search=='\t' || *search=='_'){
				if(spacech)
					continue;
				else{
					chname[i++]='_';
					spacech=1;
				}
			}
			else if (!isSpaceChar(*search)){
				spacech=0;
				chname[i++]=*search;
			}
		}
		chname[i]='\0';
		

		if((sscanf(sec[0], "0x%x", &sid) == 1 && sscanf(sec[1], "0x%x", &onid) == 1 && sscanf(sec[2], "0x%x", &tsid) == 1) || (sscanf(sec[0], "%d", &sid) == 1 && sscanf(sec[1], "%d", &onid) == 1 && sscanf(sec[2], "%d", &tsid) == 1))
		{
			      tvNameMap::iterator it=tvnmap.find(chname);
			      if(it!=tvnmap.end() && it->second != uniqueEPGKey(sid,onid,tsid)){
			      		tvmap[uniqueEPGKey(sid,onid,tsid)]=it->second;
					tvamap[uniqueEPGKey(sid,onid,tsid)]=chname;
			      		tvnmap[chname]=it->second;
			      }
			      else{
			      		tvnmap[chname]=uniqueEPGKey(sid,onid,tsid);
					tvamap[uniqueEPGKey(sid,onid,tsid)]=chname;
			      }
		}
	}
	free(line);
	fclose(f);
	return 0;
	
}

int epg::save_tvmap(eString mapfile,int mode)
{
	FILE *f = fopen(mapfile.c_str(), "wt");
	if(!f)return -1;

	for(eventCache::iterator it=eventDB.begin();it!=eventDB.end();it++){
		timeMap *tmMap=&(it->second.second);
		eString chname;
		for(timeMap::iterator i=tmMap->begin(); i!=tmMap->end();i++){
			chname.clear();
			const eit_event_struct* eit=i->second->get_v5();
			EITEvent ev( eit, (it->first.tsid<<16)|it->first.onid, i->second->type,autofix);
			eString desc=ev.ShortEventText.trim()+" "+ev.ExtendedEventText.trim();
//			printf("[CHANNEL]%s\n",desc.c_str());
			unsigned int pos=desc.find(":");
			if(pos != std::string::npos){
				chname=XML_ENCODE(desc.left(pos).trim());
				if(chname.size()<=45)
					break;
			}
		}
		if(chname=="")continue;
		if(!mode){
			tvAllNameMap::iterator ait=tvamap.find(it->first);
			if(ait == tvamap.end())
				tvamap[it->first]=chname;
		}
		else{
			tvNameMap::iterator nit=tvnmap.find(chname);
			if(nit==tvnmap.end())
				tvnmap[chname]=it->first;
		}

	}

	if(!mode)
		for(map<uniqueEPGKey,eString>::iterator mIt=tvamap.begin();mIt != tvamap.end();++mIt){
			fprintf(f,"%d:%d:%d=%s\n",mIt->first.sid,mIt->first.onid,mIt->first.tsid,mIt->second.c_str());
		
		}
	else
		for(map<eString, uniqueEPGKey>::iterator mIt=tvnmap.begin();mIt != tvnmap.end();++mIt){
			fprintf(f,"%d:%d:%d=%s\n",mIt->second.sid,mIt->second.onid,mIt->second.tsid,mIt->first.c_str());
		
		}
	fclose(f);
	
	return 0;

}

int epg::loadepg_from_xmltv(eString epgfile)
{
	FILE *f=fopen(epgfile.c_str(),"rt");
	if(!f)return -1;

	if(tvnmap.size() == 0)
		load_tvmap("tvmap.dat");
	
	map<__u16,eString> xmltvmap;
	map<__u16,uniqueEPGKey> xmltv_tsonid_map;
	
	char temp[tempsize];
	SP_XmlDomParser parser;
	fseek(f,0,SEEK_SET);
	while (!feof(f)){		
		int len=fread(temp,1,tempsize,f);		
		parser.append(temp,len);
	}
	fseek(f,0,SEEK_SET);
	
	//开始解析并载入epg
	int encode=0x15;
	int total=0;
	SP_DomIterator iter( parser.getDocument() );
	for( const SP_XmlNode * node = iter.getNext();NULL != node;node = iter.getNext() ) {
		switch(node->getType()){
		case SP_XmlNode::eDOCDECL:{
			const char * encoding=((SP_XmlDocDeclNode*)node)->getEncoding();
			if (strncasecmp(encoding, "gb2312",6) == 0 || strcasecmp(encoding, "gbk") == 0 || strcasecmp(encoding, "gb18030") == 0)
				encode=GB18030_ENCODING;
			else if (strcasecmp(encoding, "big5") == 0)
				encode=BIG5_ENCODING;
			else if (strcasecmp(encoding,"UTF8") == 0 || strcasecmp(encoding,"UTF-8") == 0 )
				encode=UTF8_ENCODING;
			break;
			}
		case SP_XmlNode::eELEMENT :{
			const char *iname=((SP_XmlElementNode*)node)->getName();

			//建立频道id映射表
			if(strcmp(iname,"channel")==0){
				const char* schannelid=((SP_XmlElementNode*)node)->getAttrValue("id");
				const char* stsonid=((SP_XmlElementNode*)node)->getAttrValue("tsonid");
				int channelid=atoi(schannelid);
				if(!channelid)continue;
				
				uniqueEPGKey tsonid;
				if(stsonid)
				   if(3==sscanf(stsonid,"0x%x:0x%x:0x%x",&tsonid.sid,&tsonid.onid,&tsonid.tsid) 
				      || 3==sscanf(stsonid,"%d:%d:%d",&tsonid.sid,&tsonid.onid,&tsonid.tsid))
					xmltv_tsonid_map[channelid]=tsonid;
					
				const SP_XmlNodeList * children=((SP_XmlElementNode*)node)->getChildren();
				int childcount=children->getLength();
				for(int j=0;j<childcount;j++){
					const char *childname=((SP_XmlElementNode*)(children->get(j)))->getName();
					if(strcmp(childname,"display-name")!=0)continue;
					const SP_XmlNodeList * tchild=((SP_XmlElementNode*)(children->get(j)))->getChildren();
					const char * text;
					if (tchild->getLength() && tchild->get(0)->getType()==SP_XmlNode::eCDATA)
						text=((SP_XmlCDataNode*)(tchild->get(0)))->getText();
					else 
						text="";
					xmltvmap[channelid]=text;	
				}			
			}
			else if(strcmp(iname,"programme")==0){	//EPG数据
				const char* schannelid=((SP_XmlElementNode*)node)->getAttrValue("channel");
				const char* sstart=((SP_XmlElementNode*)node)->getAttrValue("start");
				const char* sstop=((SP_XmlElementNode*)node)->getAttrValue("stop");
				const char* seventid=((SP_XmlElementNode*)node)->getAttrValue("event_id");

				struct event_data_struct eds;
				
				if(seventid !=NULL)eds.event_id=atoi(seventid);

				int channelid=atoi(schannelid);
				if(!channelid)continue;

				eString chname;
				map<__u16,eString>::iterator xit=xmltvmap.find(channelid);
				if(xit!=xmltvmap.end())
					 chname=xit->second;

				map<__u16,uniqueEPGKey>::iterator xkit=xmltv_tsonid_map.find(channelid);
				if(xkit != xmltv_tsonid_map.end()){
					eds.tsonid=xkit->second;
				}
				else{
					if(!chname)continue;
					tvNameMap::iterator itTvmap=tvnmap.find(chname);
					if(itTvmap==tvnmap.end())continue;
					eds.tsonid=itTvmap->second;
				}				
				time_t start_time=mktimet_from_xmltv(sstart);
				time_t stop_time=mktimet_from_xmltv(sstop);
				eds.start_time=start_time;
				eds.duration=stop_time-start_time;
				eds.encode=UTF8_ENCODING;
				const SP_XmlNodeList * children=((SP_XmlElementNode*)node)->getChildren();
				int childcount=children->getLength();
//				printf("[%s]%6s %s %s \n", iname,schannelid,sstart,sstop );
				for(int j=0;j<childcount;j++){
					const char *childname=((SP_XmlElementNode*)(children->get(j)))->getName();
					const SP_XmlNodeList * tchild=((SP_XmlElementNode*)(children->get(j)))->getChildren();
					const char * text;
					if (tchild->getLength() && tchild->get(0)->getType()==SP_XmlNode::eCDATA)
						text=((SP_XmlCDataNode*)(tchild->get(0)))->getText();
					else 
						text="";
					
					if(strcmp(childname,"title")==0)
						eds.title=convertDVBUTF8((const unsigned char *)text,strlen(text),encode);
					else if(strcmp(childname,"desc")==0){
						eds.desc=convertDVBUTF8((const unsigned char *)text,strlen(text),encode);

						if(chname &&( eds.desc.size()<chname.size() || strncmp(chname.c_str(),eds.desc.c_str(),chname.size() ) ))
							eds.desc=chname+":"+eds.desc;
						}
					}
				if(eds.title.size() == 0)continue;
				loadepg_from_event_struct(eds);
				total++;
				}
			}
			break;		
		
		}
	}
	printf("[EPGM] %d events read from xmltv %s\n", total, epgfile.c_str());

	if( NULL != parser.getError() ) {
		printf( "\n\nerror: %s\n", parser.getError() );
	}
	xmltvmap.clear();
	xmltv_tsonid_map.clear();
	fclose(f);
	return 0;
}

int epg::saveepg_to_xmltv(eString epgfile,int bomMode)
{
	FILE *f=fopen(epgfile.c_str(),"wt");
	if(!f)return -1;

	char tzs[20];
	if(timeoff>0)
		sprintf(tzs,"-%02d%02d",(int)timeoff/3600,(int)(timeoff %3600)/60);	
	else
		sprintf(tzs,"+%02d%02d",(int)-timeoff/3600,(int)-(timeoff % 3600)/60);

	if(bomMode)
		fprintf(f,"\xEF\xBB\xBF");

	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
		  "<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n\n"\
		  "<tv generator-info-name=\"opentv-xmltv 0.06\">\n");

	map<uniqueEPGKey,__u16> xmltv_tsonid_id_map;

	int no=1;
	for(eventCache::iterator it=eventDB.begin();it!=eventDB.end();it++,no++){
		timeMap *tmMap=&(it->second.second);
//		printf("---[  tsonid=%d:%d:%d  ] ---------------------\n",it->first.sid,it->first.onid,it->first.tsid);

		eString chname;
		for(timeMap::iterator i=tmMap->begin(); i!=tmMap->end();i++){
			chname.clear();
			const eit_event_struct* eit=i->second->get_v5();
			EITEvent ev( eit, (it->first.tsid<<16)|it->first.onid, i->second->type,autofix);
			eString desc=ev.ShortEventText.trim()+" "+ev.ExtendedEventText.trim();
//			printf("[CHANNEL]%s\n",desc.c_str());
			unsigned int pos=desc.find(":");
			if(pos != std::string::npos){
				chname=XML_ENCODE(desc.left(pos).trim());
				if(chname.size()<=45)
					break;
			}
		}
		if(chname=="")
			chname=chname.sprintf((char *)"%04X_%04X_%04X",it->first.sid,it->first.onid,it->first.tsid);

		xmltv_tsonid_id_map[it->first]=no;

		fprintf(f,"   <channel id=\"%d\" tsonid=\"%d:%d:%d\">\n"\
			  "        <display-name>%s</display-name>\n"\
			  "   </channel>\n",no,it->first.sid,it->first.onid,it->first.tsid,chname.c_str());
	}
	int cnt=0;
	for(eventCache::iterator it=eventDB.begin();it!=eventDB.end();it++){
		timeMap *tmMap=&(it->second.second);
		for(timeMap::iterator i=tmMap->begin();i!=tmMap->end();i++,cnt++){
			const eit_event_struct* eit=i->second->get_v5();
			EITEvent ev( eit, (it->first.tsid<<16)|it->first.onid, i->second->type,autofix);

			map<uniqueEPGKey,__u16>::iterator itKeyNo=xmltv_tsonid_id_map.find(it->first);
			if(itKeyNo == xmltv_tsonid_id_map.end())continue;

			__u16 channelid=itKeyNo->second;

			struct tm *pstart,*pend;
			char tmp[256];
			const time_t starttime=ev.start_time-timeoff;
			const time_t endtime=ev.start_time+ev.duration-timeoff;
			pstart=localtime(&starttime);
			strftime(tmp,200,"%Y%m%d%H%M%S",pstart);
			eString start_time(tmp);

			pend=localtime(&endtime);
			
			strftime(tmp,200,"%Y%m%d%H%M%S",pend);
			eString end_time(tmp);

			eString desc=ev.ShortEventText.trim();
			if(desc.size())desc+=" ";
			desc+=ev.ExtendedEventText.trim();

			unsigned int pos=desc.find(":");
			eString chname,desc2;
			if(pos != std::string::npos){
				chname=desc.left(pos+1).trim();
//				printf("chname=%s\n",chname.c_str());
				if(chname.size()<=46 and chname.size()>0){
				    desc2=desc.strReplace(chname.c_str(),"",UTF8_ENCODING).trim();
				    desc=desc2;
				}
			}

			if(desc==".")
				desc="";
			else{
				eString::iterator dit=desc.begin();
				int len=0;  
				for(; dit != desc.end(); dit++,len++) 
					if(!isSpaceChar(*dit) && (*dit)!='.'){
						desc.erase(0,len);
						break;
					}
				if(dit == desc.end())
					desc.erase(0);
			}
			desc=XML_ENCODE(desc.trim());
			eString title=XML_ENCODE(ev.ShortEventName).trim();
			fprintf(f,"   <programme channel=\"%d\" start=\"%s %s\" stop=\"%s %s\"",channelid,start_time.c_str(),tzs,end_time.c_str(),tzs);


			if(debug){
				fprintf(f," event_id=\"%d\"  count=\"%d\"",i->second->getEventID(),cnt);
				printf("   <programme channel=\"%d\" start=\"%s %s\" stop=\"%s %s\" event_id=\"%d\" count=%d>\n",channelid,start_time.c_str(),tzs,end_time.c_str(),tzs,i->second->getEventID(),cnt);
				printf(   "        <title lang=\"zh\">%s</title>\n"\
					  "        <desc  lang=\"zh\">%s</desc>\n"\
					  "   </programme>\n",title.c_str(),desc.c_str());
				}

			fprintf(f, ">\n"\
				  "        <title lang=\"zh\">%s</title>\n"\
				  "        <desc  lang=\"zh\">%s</desc>\n"\
				  "   </programme>\n",title.c_str(),desc.c_str());
		}
	}
	xmltv_tsonid_id_map.clear();
	printf("[xmltv]%d programme writed!\n",cnt);
	fprintf(f,"</tv>");
	
	return 0;
}

epg::~epg()
{
	tvnmap.clear();
	tvamap.clear();
	tvmap.clear();
	eventDB.clear();
}

int epg::dispepg()
{
	int i=0;
	int total=0;
	for(eventCache::iterator it=eventDB.begin();it!=eventDB.end();it++,i++){
		timeMap *tmMap=&(it->second.second);
		printf("---[  tsonid=%d:%d:%d  ] ---------------------\n",it->first.sid,it->first.onid,it->first.tsid);
		for(timeMap::iterator i=tmMap->begin();i!=tmMap->end();i++){
			total++;
			eit_event_struct* eit=i->second->get_v5();
//			if(debug)
//				utils_dump("EITData:",(__u8 *)eit,getINT8(HILO(eit->descriptors_loop_length)) + EIT_LOOP_SIZE,1);
//			if(total
//			utils_dump("EITData:",(unsigned char *)i->second->EITdata,i->second->getSize(1),1);
				
			EITEvent ev( eit, (it->first.tsid<<16)|it->first.onid, i->second->type,autofix);

#ifndef __WIN32__
			eString ShortEventName=ev.ShortEventName;
			eString ShortEventText=ev.ShortEventText;
			eString ExtendedEventText=ev.ExtendedEventText;
#else
			eString ShortEventName=UTF8ToGB2312((const unsigned char *)ev.ShortEventName.c_str(),ev.ShortEventName.size());
			eString ShortEventText=UTF8ToGB2312((const unsigned char *)ev.ShortEventText.c_str(),ev.ShortEventText.size());
			eString ExtendedEventText=UTF8ToGB2312((const unsigned char *)ev.ExtendedEventText.c_str(),ev.ExtendedEventText.size());
#endif
			ShortEventName=ShortEventName.trim();
			ShortEventText=ShortEventText.trim();
			ExtendedEventText=ExtendedEventText.trim();

			if(debug)
			     printf("i->first=%ld duration=%d first+duration=%ld now=%ld\n",i->first,i->second->getDuration(),i->first+i->second->getDuration(),time(0));


			printf("%d.\t%16s %4d:%02d\tN:%s",total,strtime(ev.start_time).c_str(),ev.duration/60,(ev.duration % 60),ShortEventName.c_str());

			if(debug && time(0)>(i->first+i->second->getDuration()))
				printf("\t[OLD EVENT]");

			printf("\n\t\tT:%s\n\t\tX:%s\n",ShortEventText.c_str(),ExtendedEventText.c_str());

		}
	}
	printf("eventDB::size=%d\n",eventDB.size());
	printf("Total event:%d\n",total);

//======================================================
	

//======================================================
	return 0;
}




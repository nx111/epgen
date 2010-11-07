#include <stdio.h>
#include <sys/time.h>
#include "dvb.h"
#include "type.h"
#include "endian.h"
#include "epg.h"

#define HILO(x) (x##_hi << 8 | x##_lo)

int eventData::CacheSize=0;
__u8 eventData::data[8192];

extern int running_endian;
extern int data_endian;

descriptorMap eventData::descriptors;
descriptorMap eventData::descriptors_target;

unsigned int crc32_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

void utils_dump(const char *label, unsigned char *data, int len,int option) 
 { 
         int i,j; 
	 printf("\n%s",label); 
	 if(option & 0x1)printf("\n");	//换行
	 
	 for(i = 0,j=0; i < len; i++,j++) { 
	         printf("%02X ", data[i]);
	         if((j % 8) ==7)printf(" ");
	         if(j==31){
	         	printf("\n");
	         	j=-1;
	         } 
	 } 
	 printf("\n [len=0x%02X]\n", len); 
	 return;
 } 

void debug_eit(eit_event_struct * EIT)
{
	struct eit_event_struct eit;
	memcpy(&eit,EIT,12);
	adjust_eit_event_endian((__u8*)&eit);
	time_t start_time=parseDVBtime(eit.start_time_1, eit.start_time_2, eit.start_time_3,eit.start_time_4, eit.start_time_5);
	int duration=fromBCD(eit.duration_1)*3600+fromBCD(eit.duration_2)*60+fromBCD(eit.duration_3);
	printf("DEBUG: EventID:%04X start_time:%s duration:%d \n",HILO(eit.event_id),strtime(start_time).c_str(),duration); 
}

void adjust_eit_event_endian(__u8 *data,int endian)
{
	if(B_ENDIAN==endian)return;
	__u8 a=data[10] & 0x0F;
	__u8 b=data[10] & 0xF0;
	data[10]=(a<<4 | b>>4);
}

void adjust_eit_extented_descriptor_endian(__u8 *data,int endian)
{
	if(B_ENDIAN==endian)return;
	__u8 a=data[2] & 0x0F;
	__u8 b=data[2] & 0xF0;
	data[2]=(a<<4 | b>>4);
}

int fromBCD(int bcd)
{
	if ((bcd&0xF0)>=0xA0)
		return -1;
	if ((bcd&0xF)>=0xA)
		return -1;
	return ((bcd&0xF0)>>4)*10+(bcd&0xF);
}

int toBCD(int dec)
{
	if (dec >= 100)
		return -1;
	return int(dec/10)*0x10 + dec%10;
}

int makeDVBtime(unsigned char *out,time_t time)
{
	struct tm *m;
	m=gmtime(&time);
	unsigned int M=m->tm_mon+1;
	unsigned int L=(M==1 || M==2)?1:0;
	unsigned int D=m->tm_mday;
	unsigned int Y=m->tm_year;
	unsigned int mjd=14956+D+(int)((Y-L)*365.25)+(int)((M+1+12*L)*30.6001);
	out[0]=mjd>>8;
	out[1]=mjd & 0x00ff;
	out[2]=toBCD(m->tm_hour);
	out[3]=toBCD(m->tm_min);
	out[4]=toBCD(m->tm_sec);
	return 0;
}

time_t parseDVBtime(__u8 t1, __u8 t2, __u8 t3, __u8 t4, __u8 t5)
{
	tm t;
	t.tm_sec=fromBCD(t5);
	t.tm_min=fromBCD(t4);
	t.tm_hour=fromBCD(t3);
	int mjd=(t1<<8)|t2;
	int k;

	t.tm_year = (int) ((mjd - 15078.2) / 365.25);
	t.tm_mon = (int) ((mjd - 14956.1 - (int)(t.tm_year * 365.25)) / 30.6001);
	t.tm_mday = (int) (mjd - 14956 - (int)(t.tm_year * 365.25) - (int)(t.tm_mon * 30.6001));
	k = (t.tm_mon == 14 || t.tm_mon == 15) ? 1 : 0;
	t.tm_year = t.tm_year + k;
	t.tm_mon = t.tm_mon - 1 - k * 12;
	t.tm_mon--;

	t.tm_isdst =  -1;
//	t.tm_gmtoff = 0;

	return mktime(&t);
//	return my_mktime(&t)-timezone;
}



eventData::eventData(const eit_event_struct* e, int size, int type,int source)
	:EITdata(NULL),EITdata_v7(NULL),ByteSize(size&0xFF), ByteSize_v7(0),type(type&0xFF)
{
	this->source=source;
	if(source==srPLI_EPGDAT_LE || source==srGEMINI_EPGDAT_LE)
		endian=L_ENDIAN;
	else
		endian=B_ENDIAN;

	if (!e)
		return;

	if (!size)
	{	
		size = HILO(e->descriptors_loop_length) + EIT_LOOP_SIZE;
		ByteSize=size&0xFF;
	}
	EITdata = new __u8[size];
	CacheSize+=size;
	memcpy(EITdata, (__u8*)e, size);
}

eit_event_struct* eventData::get_v5(int targetEndian)
{
	if (source==srPLI_EPGDAT_BE || source==srPLI_EPGDAT_LE){
		if(targetEndian!=X_ENDIAN){
			adjust_eit_event_endian(EITdata,targetEndian);
			endian=targetEndian;
		}
		else
			adjust_eit_event_endian(EITdata,endian);
		
		return (eit_event_struct *)EITdata;
	}

	int pos = EIT_LOOP_SIZE;
	int tmp = ByteSize-10;
	memcpy(data, EITdata, 10);
	int descriptors_length=0;
	__u32 *p = (__u32*)(EITdata+10);
	while(tmp>3 )
	{
		__u32 descriptor_id=getINT32(*p++,endian);
		descriptorMap::iterator it =
			descriptors.find(descriptor_id);
		if ( it != descriptors.end() )
		{
			int b = it->second.second[1]+2;

			if((pos+b)>eventData_TmpSize)break;

			__u32 crc = 0;
			int cnt=0,optr=0;
			while(cnt++ < (it->second.second[1]+2))
				crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ it->second.second[optr++]) & 0xFF];
			
			__u32 crc_b=getINT32(crc,B_ENDIAN);
			__u32 crc_l=getINT32(crc,L_ENDIAN);
			if(descriptor_id != crc_b && descriptor_id != crc_l)break;
		
			memcpy(data+pos, it->second.second, b );
			pos += b;
			descriptors_length += b;
		}
		tmp-=4;
	}
	((eit_event_struct *)data)->descriptors_loop_length_hi = (descriptors_length >> 8) & 0x0F;
	((eit_event_struct *)data)->descriptors_loop_length_lo = descriptors_length & 0xFF;

//	printf("[debug]descriptors_length=0x%x\n",descriptors_length);

	if(targetEndian!=X_ENDIAN)
		adjust_eit_event_endian(data,targetEndian);
	else
		adjust_eit_event_endian(data,endian);
	
	return (eit_event_struct*)data;
	
}

__u8* eventData::get_v7(int *looplen,int targetEndian)
{
	if(EITdata_v7)
		return EITdata_v7;

	int toEndian=targetEndian;
	if(toEndian==X_ENDIAN)
		toEndian=data_endian;
	__u8 *descrData;
	memcpy(data, EITdata, 10);

	eString ShortEventName="",ShortEventText="",ExtendEventText="";

	if (source!=srPLI_EPGDAT_BE && source!=srPLI_EPGDAT_LE){
		int tmp=ByteSize-10;
		__u32 *p = (__u32*)(EITdata+10);
		while(tmp>3 )	//get Event Name and Descr Text
		{
			descriptorMap::iterator it =
				descriptors.find(getINT32(*p++));
			
			if ( it != descriptors.end() )
			{
				Descriptor *descr = Descriptor::create((descr_gen_struct *)(it->second.second),0,0,0);
//				utils_dump("it->second.second:",it->second.second,it->second.second[1]);
				if ( descr->Tag() == DESCR_SHORT_EVENT )
				{
					ShortEventDescriptor *sdescr = (ShortEventDescriptor*)descr;
					ShortEventName = sdescr->event_name;
					ShortEventText = sdescr->text;
					delete descr;
				}
				else if ( descr->Tag() == DESCR_EXTENDED_EVENT )
				{
					ExtendedEventDescriptor *edescr = (ExtendedEventDescriptor*) descr;
					eString txt=edescr->text;
					if(txt.at(0)<0x20 && ExtendEventText.size())
						txt.erase(0,1);
		//			printf("[ExtendedEventText]%s\n",edescr->text.c_str());
					ExtendEventText+=txt;
					delete descr;
				}
				else   
				{
					delete descr;
					break;
				}				

			}
			tmp-=4;
		}

		
	}else {

		int len=HILO(((const eit_event_struct *)EITdata)->descriptors_loop_length);
		int ptr=0;
		while (ptr<len)
		{
			descr_gen_t *d=(descr_gen_t*) (((__u8*)(EITdata+12))+ptr);
			Descriptor *descr = Descriptor::create(d,0,0,0);

			if ( descr->Tag() == DESCR_SHORT_EVENT )
			{
				ShortEventDescriptor *sdescr=(ShortEventDescriptor *)descr;
				ShortEventName=sdescr->event_name;
				ShortEventText=sdescr->text.mid(0,250,UTF8_ENCODING);
				delete descr;
			}
			else if ( descr->Tag() == DESCR_EXTENDED_EVENT )
			{
				ExtendedEventDescriptor *edescr = (ExtendedEventDescriptor*) descr;
				eString txt=edescr->text;
				if(txt.at(0)<0x20 && ExtendEventText.size())
					txt.erase(0,1);
				ExtendEventText+=txt;
				delete descr;
			}
			else   
			{
				delete descr;
				break;
			}				
			ptr+=d->descriptor_length+2;
		}
	}
	ExtendEventText=convertDVBUTF8((const unsigned char *)(ExtendEventText.c_str()),ExtendEventText.size());

	__u32 *p = (__u32*)(data+10);
	int ptr=10;

//	ShortEvent descriptor
	descrData=new __u8[ShortEventName.size()+ShortEventText.size()+7+2];

	
	int optr=0;
	descrData[optr++]='\x4D';	
	descrData[optr++]=ShortEventName.size()+ShortEventText.size()+7;
	descrData[optr++]='e';
	descrData[optr++]='n';
	descrData[optr++]='g';
	descrData[optr++]=ShortEventName.size()+1;
	descrData[optr++]=UTF8_ENCODING;
	memcpy(descrData+optr,ShortEventName.c_str(),ShortEventName.size());
	optr+=ShortEventName.size();

	descrData[optr++]=ShortEventText.size()+1;
	descrData[optr++]=UTF8_ENCODING;
	memcpy(descrData+optr,ShortEventText.c_str(),ShortEventText.size());

	__u32 crc = 0;
	int cnt=0;
	optr=0;
	while(cnt++ < (descrData[1]+2))
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ descrData[optr++]) & 0xFF];

	descriptorMap::iterator it = descriptors_target.find(crc);
	if ( it == descriptors_target.end() )
	{
		descriptors_target[crc]= descriptorPair(1,descrData);
	}
	else{
		++it->second.first;
		delete []descrData;
	}
	(*p++)=getINT32(crc,toEndian);
	ptr+=sizeof(__u32);

//	Extend Event Descriptor
	__u8 last_descriptor_number=0,descriptor_number=0;
	for(unsigned int pos=0;pos<ExtendEventText.size();)
	   {
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
		eString eDesc=ExtendEventText.mid(pos,maxlen,UTF8_ENCODING);
		if(0==eDesc.size())break;
		pos+=eDesc.size();
//		printf("eDesc[0x%x]%s\n",eDesc.size(),eDesc.c_str());
		
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
		descrData[optr++]=UTF8_ENCODING;

		adjust_eit_extented_descriptor_endian(descrData,toEndian);
		memcpy(descrData+optr,eDesc.c_str(),eDesc.size());

		crc = 0;
		cnt=0;
		optr=0;
		while(cnt++ < (descrData[1]+2))
			crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ descrData[optr++]) & 0xFF];

		it = descriptors_target.find(crc);
		if ( it == descriptors_target.end() )
		{
			descriptors_target[crc]=descriptorPair(1,descrData);
		}
		else{
			++it->second.first;
			delete []descrData;
		}
		(*p++)=getINT32(crc,toEndian);
		ptr+=sizeof(__u32);

		last_descriptor_number=descriptor_number;
		descriptor_number++;
	
	}

	if(looplen)
		*looplen=ptr;

	ByteSize_v7=ptr;
	EITdata_v7=new __u8[ptr];
	memcpy(EITdata_v7,data,ptr);
	ByteSize_v7=ptr;

	return EITdata_v7;
	
}


int eventData::getSize(int boolEITdataSize)
{
	if (!EITdata) return 0;
	if(boolEITdataSize || !EITdata_v7)
		return ByteSize;
	
	return ByteSize_v7;

}

eventData::~eventData()
{
	if ( ByteSize && (source==srGEMINI_EPGDAT_LE || source==srGEMINI_EPGDAT_LE ))
	{
		CacheSize -= ByteSize;
		__u32 *d = (__u32*)(EITdata+10);
		ByteSize -= 10;
		while(ByteSize>3)
		{
			descriptorMap::iterator it =
				descriptors.find(getINT32(*d++));
			if ( it != descriptors.end() )
			{
				descriptorPair &p = it->second;
				if (!--p.first) // no more used descriptor
				{
					CacheSize -= it->second.second[1];
					delete [] it->second.second;  	// free descriptor memory
					descriptors.erase(it);	// remove entry from descriptor map
				}
			}
			ByteSize -= 4;
		}
	}

	if(ByteSize_v7)
	{
		__u32 *d = (__u32*)(EITdata_v7+10);	
		ByteSize_v7 -= 10;
		while(ByteSize_v7>3)
		{
			descriptorMap::iterator it =
				descriptors_target.find(getINT32(*d++));
			if ( it != descriptors_target.end() )
			{
				descriptorPair &p = it->second;
				if (!--p.first) // no more used descriptor
				{
					CacheSize -= it->second.second[1];
					delete [] it->second.second;  	// free descriptor memory
					descriptors_target.erase(it);	// remove entry from descriptor map
				}
			}
			ByteSize_v7 -= 4;
		}
		delete [] EITdata_v7;

	}
	delete [] EITdata;
}

void eventData::load(FILE *f,int source)
{
	if (source!=srGEMINI_EPGDAT_BE && source!=srGEMINI_EPGDAT_LE )
		return;


	int size=0,i;
	int id=0;
	__u8 header[2];
	descriptorPair p;
	fread(&size, sizeof(int), 1, f);
	size=getINT32(size);
	i=size;
	while(i)
	{
		fread(&id, sizeof(__u32), 1, f);
		id=getINT32(id);
		fread(&p.first, sizeof(__u16), 1, f);
		p.first=getINT16(p.first);
		__u16 reserverd;
		fread(&reserverd,sizeof(__u16),1,f);

		fread(header, 2, 1, f);
		int bytes = header[1]+2;
		p.second = new __u8[bytes];
		p.second[0] = header[0];
		p.second[1] = header[1];
		fread(p.second+2, bytes-2, 1, f);
		descriptors[id]=p;
		--i;
		CacheSize+=bytes;
//		utils_dump("[Descriptor]",p.second,bytes);
	}
	printf("descriptors:%d descriptors loaded!\n",size);
	
}

void eventData::save(FILE *f,int source)
{
	if (source!=srGEMINI_EPGDAT_BE && source!=srGEMINI_EPGDAT_LE )
		return;
	int size=descriptors_target.size();
	int toEndian=(source != srGEMINI_EPGDAT_LE)?B_ENDIAN:L_ENDIAN;
	int osize=getINT32(size,toEndian);
	descriptorMap::iterator it(descriptors_target.begin());
	fwrite(&osize, sizeof(int), 1, f);
	while(it!=descriptors_target.end())
	{
		__u32 id=getINT32(it->first,toEndian);
		__u16 shareid=getINT16(it->second.first,toEndian);
		__u16 reserverd=0;
		fwrite(&id, sizeof(__u32), 1, f);
		fwrite(&shareid, sizeof(__u16), 1, f);
		fwrite(&reserverd,sizeof(__u16),1,f);
		fwrite(it->second.second, it->second.second[1]+2, 1, f);
		++it;
	}
	printf("descriptors:%d descriptors writed!\n",size);
}

Descriptor *Descriptor::create(descr_gen_t *descr, int tsidonid, int type,int encode)
{
	switch (descr->descriptor_tag)
	{
	case DESCR_SHORT_EVENT:
		return new ShortEventDescriptor(descr,tsidonid,encode);
	case DESCR_EXTENDED_EVENT:
		return new ExtendedEventDescriptor(descr,tsidonid,encode);
	default:
		return new UnknownDescriptor(descr);
	}
}

UnknownDescriptor::UnknownDescriptor(descr_gen_t *descr,int encode)
	:Descriptor(descr), data(0)
{
	this->encode=encode;
	if ( len > 2 )
	{
		data = new __u8[len-2];
		memcpy(data, (__u8*) (descr+1), len-2);
	}
}

UnknownDescriptor::~UnknownDescriptor()
{
	if (len>2)
	{
		delete [] data;
		data=0;
	}
}

ShortEventDescriptor::ShortEventDescriptor(descr_gen_t *descr, int tsidonid,int encode)
	:Descriptor(descr), tsidonid(tsidonid)
{
	this->encode=encode;
	init_ShortEventDescriptor(descr);
}

void ShortEventDescriptor::init_ShortEventDescriptor(descr_gen_t *descr)
{
	__u8 *data=(__u8*)descr;
	memcpy(language_code, data+2, 3);
	int ptr=5;
	int len=data[ptr++];
	
	event_name=convertDVBUTF8((unsigned char*)data+ptr, len, encode, tsidonid);

	ptr+=len;
	
	if((ptr-2)>=descr->descriptor_length)return;

	len=data[ptr++];
	text=convertDVBUTF8((unsigned char*) data+ptr, len, encode, tsidonid);
	unsigned int start = text.find_first_not_of("\x0a");
	if (start > 0 && start != std::string::npos)
	{
		text.erase(0, start);
	}
	
}

ExtendedEventDescriptor::ExtendedEventDescriptor(descr_gen_t *descr, int tsidonid,int encode)
	:Descriptor(descr), tsidonid(tsidonid)
{
	this->encode=encode;
	init_ExtendedEventDescriptor(descr);
}

ItemEntry::ItemEntry(eString &item_description, eString &item)
	:item_description(item_description), item(item)
{
}

ItemEntry::~ItemEntry()
{
}

void ExtendedEventDescriptor::init_ExtendedEventDescriptor(descr_gen_t *descr)
{
	struct eit_extended_descriptor_struct *evt=(struct eit_extended_descriptor_struct *)descr;
	adjust_eit_extented_descriptor_endian((__u8*)evt);
	items.setAutoDelete(true);
	descriptor_number = evt->descriptor_number;
	last_descriptor_number = evt->last_descriptor_number;
	language_code[0]=evt->iso_639_2_language_code_1;
	language_code[1]=evt->iso_639_2_language_code_2;
	language_code[2]=evt->iso_639_2_language_code_3;

	int table=encode;

	int ptr = sizeof(struct eit_extended_descriptor_struct);
	__u8* data = (__u8*) descr;

	int length_of_items=data[ptr++];
	int item_ptr=ptr;
	int item_description_len;
	int item_len;

	while (ptr < item_ptr+length_of_items)
	{
		eString item_description;
		eString item;

		item_description_len=data[ptr++];
		item_description=convertDVBUTF8((unsigned char*) data+ptr, item_description_len, table, tsidonid );
		ptr+=item_description_len;

		item_len=data[ptr++];
		item=convertDVBUTF8((unsigned char*) data+ptr, item_len, table, tsidonid);
		ptr+=item_len;

		items.push_back(new ItemEntry(item_description, item));
	}

	int text_length=data[ptr++];
//	text=convertDVBUTF8((unsigned char*) data+ptr, text_length, table, tsidonid);
	text=eString((const char*)data+ptr,text_length);
	ptr+=text_length;
}

EITEvent::EITEvent(const eit_event_struct *event, int tsidonid, int type,int autofix)
{
	this->autofix=autofix;
	init_EITEvent(event, tsidonid);
}
void EITEvent::init_EITEvent(const eit_event_struct *event, int tsidonid)
{
	event_id=HILO(event->event_id);
	if (event->start_time_5!=0xFF)
		start_time=parseDVBtime(event->start_time_1, event->start_time_2, event->start_time_3,event->start_time_4, event->start_time_5);
	else
		start_time=-1;
	if ((event->duration_1==0xFF) || (event->duration_2==0xFF) || (event->duration_3==0xFF))
		duration=-1;
	else
		duration=fromBCD(event->duration_1)*3600+fromBCD(event->duration_2)*60+fromBCD(event->duration_3);
	int ptr=0;
	int len=HILO(event->descriptors_loop_length);
	__u8 encode=0;
	int doneShortEvent=0;
	while (ptr<len)
	{
		//fix for extended event data
		__u8 *pdescrData=(((__u8*)(event+1))+ptr);
		int descrLen=getDescriptorLen(pdescrData,len-ptr);

		descr_gen_t *d=(descr_gen_t*)pdescrData;

//		utils_dump("[Descriptor]\n",(__u8*)d,d->descriptor_length+2,0);

		Descriptor *descr = Descriptor::create(d,tsidonid,0,encode);
		if ( descr->Tag() == DESCR_SHORT_EVENT )
		{
			ShortEventDescriptor *sdescr = (ShortEventDescriptor*)descr;
			ShortEventName = sdescr->event_name;
			ShortEventText = sdescr->text;
			doneShortEvent=1;
			
//			printf("[ShortEventName]%s\t [ShortEventText]%s\n",sdescr->event_name.c_str(),sdescr->text.c_str());
			delete descr;
		}
		else if ( descr->Tag() == DESCR_SHORT_EVENT && doneShortEvent);	//alway done shortEvent,skip it
		else if ( descr->Tag() == DESCR_EXTENDED_EVENT )
		{
			ExtendedEventDescriptor *edescr = (ExtendedEventDescriptor*) descr;
			eString txt=edescr->text;
			
//			eString	txt=eString((const char*)(pdescrData+8+pdescrData[6]),descrLen-8-pdescrData[6]);
			if((unsigned char)(txt.at(0))<0x20 && ExtendedEventText.size())
				txt.erase(0,1);
//			printf("[ExtendedEventText]%s\n",edescr->text.c_str());
			ExtendedEventText+=txt;
			delete descr;
		}
		else 
		{
			printf("Invalid descriptor,Tag:0x%02x, descriptor_length:0x%x len:0x%x ptr:0x%x\n",descr->Tag(),d->descriptor_length,len,ptr);
//			utils_dump("[eventData]\n",(__u8*)(event+1),len,0);
//			utils_dump("[Descriptor]\n",(__u8*)d,d->descriptor_length+2,0);
//			printf("%s\n",(__u8*)d);
			delete descr;
			break;
		}
		if(autofix)
			ptr+=descrLen;
		else
			ptr+=d->descriptor_length+2;
	}
	ExtendedEventText=convertDVBUTF8((const unsigned char *)(ExtendedEventText.c_str()),ExtendedEventText.size());
//	printf("[EITEvent]len=%d\t[N]%s\t[T]%s\n\t\%s\n",len,ShortEventName.c_str(),ShortEventText.c_str(),ExtendedEventText.c_str());

}

int EITEvent::getDescriptorLen(__u8* event,int len){
	int nlen=event[1];
	if(event[0]==DESCR_EXTENDED_EVENT || event[0]==DESCR_SHORT_EVENT){
		__u8 *p;
		nlen=5;
		for(p=event+nlen; nlen<len; p++,nlen++){
			if((nlen<(len-7) && p[0]=='\x4e' && p[3]=='e' && p[4]=='n' && p[5] == 'g') ||
			   (nlen<(len-7) && p[0]=='\x4e' && p[3]=='E' && p[4]=='N' && p[5] == 'G') ||
			   (nlen<(len-6) && p[0]=='\x4d' && p[2]=='e' && p[3]=='n' && p[4] == 'g') ||
			   (nlen<(len-6) && p[0]=='\x4d' && p[2]=='E' && p[3]=='N' && p[4] == 'G') ){
				break;
			}
		}
//		for(p=event+nlen;p>(event+6) && (*p)=='\0';p--,nlen--);
	}
	return nlen;

}


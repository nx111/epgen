#ifndef __DVB_H__
#define __DVB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ext/hash_map>

#include "type.h"
#include "estring.h"
#include "eptrlist.h"

using namespace std;

using namespace __gnu_cxx;

extern unsigned int crc32_table[256];

#define EIT_EXTENDED_EVENT_DESCRIPOR 0x4e
#define EIT_SHORT_EVENT_DESCRIPTOR 0x4d
#define EIT_SHORT_EVENT_DESCRIPTOR_SIZE 6
#define EIT_LOOP_SIZE 12
#define DESCR_SHORT_EVENT	0x4D
#define DESCR_EXTENDED_EVENT	0x4E

enum {
	PRIVATE=0,
	NOWNEXT=1,
	SCHEDULE=2,
	SCHEDULE_OTHER=4,
	SCHEDULE_MHW=8,
	SCHEDULE_FREESAT=16
};

struct eit_event_struct{		//BIG ENDIAN
	u_char	event_id_hi			: 8;
	u_char	event_id_lo			: 8;
	
	u_char	start_time_1			: 8;
	u_char	start_time_2			: 8;
	u_char	start_time_3			: 8;
	u_char	start_time_4			: 8;
	u_char	start_time_5			: 8;

	u_char	duration_1			: 8;
	u_char	duration_2			: 8;
	u_char	duration_3			: 8;

	u_char	running_status			: 3;
	u_char	free_CA_mode			: 1;
	u_char	descriptors_loop_length_hi	: 4;

	u_char	descriptors_loop_length_lo	: 8;
};



struct eit_short_event_descriptor{
	u_char	descriptor_tag			: 8;
	u_char	descriptor_length		: 8;
	
	u_char	language_code_1			: 8;
	u_char	language_code_2			: 8;
	u_char	language_code_3			: 8;

	u_char	event_name_length		: 8;
};

struct eit_extended_descriptor_struct {	//BIG ENDIAN
	u_char descriptor_tag : 8;
	u_char descriptor_length : 8;
	u_char descriptor_number : 4;
	u_char last_descriptor_number : 4;
	u_char iso_639_2_language_code_1 : 8;
	u_char iso_639_2_language_code_2 : 8;
	u_char iso_639_2_language_code_3 : 8;
};

typedef struct descr_gen_struct {
	u_char	descriptor_tag		: 8;
	u_char	descriptor_length	: 8;
}descr_gen_t;

void adjust_eit_event_endian(__u8 *data,int endian=X_ENDIAN);
void adjust_eit_extented_descriptor_endian(__u8 *data,int endian=X_ENDIAN);

void debug_eit(eit_event_struct * EIT);

struct uniqueEPGKey
{
	int sid, onid, tsid;
	uniqueEPGKey():sid(-1), onid(-1), tsid(-1){}
	uniqueEPGKey( int sid, int onid, int tsid ):sid(sid), onid(onid), tsid(tsid){}
	bool operator <(const uniqueEPGKey &a) const{return memcmp( &sid, &a.sid, sizeof(int)*3)<0;}
	operator bool() const{return !(sid == -1 && onid == -1 && tsid == -1);}
	bool operator==(const uniqueEPGKey &a) const{return !memcmp( &sid, &a.sid, sizeof(int)*3);}
	struct equal{
		bool operator()(const uniqueEPGKey &a, const uniqueEPGKey &b) const
		{
			return !memcmp( &a.sid, &b.sid, sizeof(int)*3);
		}
	};

};

struct hash_uniqueEPGKey
{
	inline size_t operator()( const uniqueEPGKey &x) const
	{
		return (x.tsid << 16) | x.sid;
	}
};


int fromBCD(int bcd);
int toBCD(int dec);
time_t parseDVBtime(__u8 t1, __u8 t2, __u8 t3, __u8 t4, __u8 t5);
int makeDVBtime(unsigned char *out,time_t time);

#define eventCache hash_map<uniqueEPGKey,pair<eventMap, timeMap>, hash_uniqueEPGKey, uniqueEPGKey::equal>
#define eventMap map<__u16, eventData*>
#define timeMap  map<time_t, eventData*>
#define descriptorPair pair<__u16,__u8*>
#define descriptorMap hash_map<__u32, descriptorPair>
#define tsonidMap hash_map<uniqueEPGKey,uniqueEPGKey,hash_uniqueEPGKey, uniqueEPGKey::equal>
#define tvNameMap map<eString,uniqueEPGKey>
#define tvAllNameMap map<uniqueEPGKey,eString>

#define eventData_TmpSize	8192
class eventData
{
	friend class epg;
private:
	__u8 * EITdata;
	__u8 * EITdata_v7;
	__u8 ByteSize,ByteSize_v7;
	static descriptorMap descriptors;
	static descriptorMap descriptors_target;

	static __u8 data[8192];
	int endian;
public:
	__u8 type;
	__u8 source;	
	static int CacheSize;
	static void load(FILE *f,int source=0);
	static void save(FILE *f,int source=0);

	eventData(const eit_event_struct* e, int size, int type,int source);
	~eventData();
	eit_event_struct* get_v5(int targetEndian=X_ENDIAN) ;
	__u8* get_v7( int *looplen,int targetEndian=X_ENDIAN);
//	operator const eit_event_struct*() const { return get();}
	int getEventID(){ return (EITdata[0] << 8) | EITdata[1]; }
	time_t getStartTime() {return parseDVBtime(EITdata[2], EITdata[3], EITdata[4], EITdata[5], EITdata[6]);}
	int getDuration(){ return fromBCD(EITdata[7])*3600+fromBCD(EITdata[8])*60+fromBCD(EITdata[9]);}
	int getSize(int boolEITdataSize=1);
	int getEndian(){return endian;};
	void setEndian(int pendian){endian=pendian;};

};

class Descriptor
{
	int tag;
protected:
	int len;
public:
	inline Descriptor(descr_gen_t *descr)
		:tag(*((__u8*)descr)), len((__u8)descr->descriptor_length)
	{
		len+=2;
		encode=0;
	};
	inline Descriptor(int tag, int len)
		:tag(tag), len(len + 2) {}
	inline virtual ~Descriptor(){};

	static Descriptor *create(descr_gen_t *data, int tsidonid = 0, int type = 0,int encode=0);
	int Tag() { return tag; }
	int encode;

};


class UnknownDescriptor: public Descriptor
{
public:
  __u8 *data;
  UnknownDescriptor(descr_gen_t *descr,int encode=0);
  ~UnknownDescriptor();
  int length() { return len-2; }
};

class ShortEventDescriptor: public Descriptor
{
	void init_ShortEventDescriptor(descr_gen_t *descr);
public:
	ShortEventDescriptor(descr_gen_t *descr, int tsidonid,int encode=0);
	ShortEventDescriptor(int len, int tsidonid): Descriptor(0x4d, len), tsidonid(tsidonid) { };
	char language_code[3];
	eString event_name;
	eString text;
	int tsidonid;
};

class ItemEntry
{
public:
	eString item_description;
	eString item;
	ItemEntry(eString &item_description, eString &item);
	~ItemEntry();
};


class ExtendedEventDescriptor: public Descriptor
{
	void init_ExtendedEventDescriptor(descr_gen_t *descr);
public:
	ExtendedEventDescriptor(descr_gen_t *descr, int tsidonid,int encode=0);
	int descriptor_number;
	int last_descriptor_number;
	char language_code[3];
	ePtrList< ItemEntry > items;
	eString text;
	int tsidonid;
};

class EITEvent
{
	void init_EITEvent(const eit_event_struct *event, int tsidonid);
	int getDescriptorLen(__u8* event,int len);
public:
	EITEvent(const eit_event_struct *event, int tsidonid, int type,int autofix=0);
	EITEvent();
	int event_id;
	int autofix;		
	time_t start_time;
	int duration;
	eString ShortEventName,ExtendedEventText,ShortEventText;
	ePtrList< Descriptor > descriptor;
};

void utils_dump(const char *label, unsigned char *data, int len,int option=0) ;
#endif

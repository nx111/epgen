#ifndef __EPG_H__
#define __EPG_H__

#include "string.h"
#include "dvb.h"



void init();
string strtime(time_t time);

struct event_data_struct{
	uniqueEPGKey tsonid;
	time_t	start_time;
	int 	event_id;
	int 	duration;
	int 	encode;
	eString  title;
	eString  desc;
	event_data_struct():tsonid(uniqueEPGKey(0,0,0)),start_time(0),event_id(-1),duration(0),encode(0),title(""),desc(""){};
};	

class epg
{
	int endian;
	int maxEventID;
	time_t timezone;
	int load_tvmap(eString mapfile);
	int loadepg_from_event_struct(event_data_struct &eds);
	int loadepg_from_xmltv(eString epgfile);
	int saveepg_to_xmltv(eString epgfile,int bomMode=0);
public:
	int debug;
	epg();
	~epg();
	int loadepg(eString epgfile,int filetype=0);
	int saveepg(eString epgfile,int targetMode=0,int bomMode=0);
	int save_tvmap(eString mapfile);
	int dispepg();
};

#endif

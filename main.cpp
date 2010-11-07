#include <time.h>
#include "lib/epg.h"
#include "lib/estring.h"

#define VERSION		"1.1.12"

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
                            "Sep", "Oct", "Nov", "Dec"};

int main(int argc,char ** argv)
{
	init();

	string infile,outfile;
	int mode=0,inputed=0;
	int wmode=srGEMINI_EPGDAT_BE;
	int bom=0,autofix=1;
	int tvmapout=0;

	for(int i=1;i<argc;i++){
		if(strcmp(argv[i],"-i")==0 && i<argc-1 ){
			infile=argv[i+1];
			i++;
		}
		else if(strcmp(argv[i],"-o")==0  && i<argc-1 ){
			outfile=argv[i+1];
			i++;
		}
		else if(strcmp(argv[i],"-d")==0 )
			mode=1;
		else if(strcmp(argv[i],"-dd")==0 )
			mode=2;
		else if(strcmp(argv[i],"-v7")==0 || strcmp(argv[i],"-v7be")==0  )
			wmode=srGEMINI_EPGDAT_BE;
		else if(strcmp(argv[i],"-v7le")==0 )
			wmode=srGEMINI_EPGDAT_LE;
		else if(strcmp(argv[i],"-v5")==0 )
			wmode=srPLI_EPGDAT_BE;
		else if(strcmp(argv[i],"-xmltv")==0 )
			wmode=srXMLTV;
		else if(strcmp(argv[i],"-bom")==0 )
			bom=1;
		else if(strcmp(argv[i],"-tvmap")==0 )
			tvmapout=1;
		else if(strcmp(argv[i],"-autofix")==0 && i<argc-1){
			autofix=atoi(argv[i+1]);
			i++;
		}
		else if(strcmp(argv[i],"-?")==0 || strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0)
			mode=-1;
	}
	
	if(argc==1 || mode==-1 || infile==""){
		int year,day,mon;
		char smon[20];
		sscanf(__DATE__,"%s %d %d",smon,&day,&year);
		for (int i = 0; i < 12; i++)
		{
		    if (!strncasecmp(smon, months[i],3))
		    {
		      mon = i + 1;
		      break;
		    }
		}


		printf("**********************************************************************\n");
		printf("*         EPG Generator v%10s                                  *\n",VERSION);
		printf("*         Author：nx111       email:gdzdz@163.com                    *\n");
		printf("*         %04d.%02d                                                    *\n",year,mon);
		printf("**********************************************************************\n");
		
		eString argv0=argv[0];
#ifndef __WIN32__
		unsigned int boff=argv0.rfind("/");
#else
		unsigned int boff=argv0.rfind("\\");
#endif
		char *basename;
		if(boff != std::string::npos)
			basename=argv[0]+boff+1;
		else
			basename=argv[0];
		printf("\nusage:  %s [-i <input epgfile>] [-o <output epgfile>] [-v5|-v7|-v7be|-v7le|-xmltv ] [-bom] [-autofix <1|0>] [-tvmap] [-d|-dd] [-?|-h|--help]\n",basename);
		return 0;
	}

	epg e; 
	e.autofix=autofix;

	if(mode==2)e.debug=1;

#ifndef __WIN32__
	unsigned int boff=infile.rfind("/");
#else
	unsigned int boff=infile.rfind("\\");
#endif
	eString infiledirname;
		if(boff != std::string::npos)
			infiledirname=infile.substr(0,boff+1);
		else
			infiledirname="";

	if(access((infiledirname+"tvmap_all.dat").c_str(),R_OK) == 0)
		e.load_tvmap(infiledirname+"tvmap_all.dat");
	else if(access((infiledirname+"tvmap.dat").c_str(),R_OK) == 0)
		e.load_tvmap(infiledirname+"tvmap.dat");

	inputed=e.loadepg(infile);


	if(outfile != "")
		e.saveepg(outfile,wmode,bom);
	if(mode)
		e.dispepg();
	if(tvmapout){
		e.save_tvmap(infiledirname+"tvmap_all.dat",0);
		e.save_tvmap(infiledirname+"tvmap.dat",1);
	}
	return 0;
}

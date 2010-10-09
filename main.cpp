#include <time.h>
#include "lib/epg.h"
#include "lib/estring.h"

#define VERSION		"1.1.8"

int main(int argc,char ** argv)
{
	init();

	string infile,outfile;
	int mode=0,inputed=0;
	int wmode=srGEMINI_EPGDAT_BE;
	int bom=0;

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
		else if(strcmp(argv[i],"-?")==0 || strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0)
			mode=-1;
	}
	
	if(argc==1 || mode==-1 || infile==""){
		struct tm *local;
		time_t t;
		t=time(NULL);
		local=localtime(&t);

		printf("**********************************************************************\n");
		printf("*         EPG Generator v%s                                       *\n",VERSION);
		printf("*         Author：nx111       email:gdzdz@163.com                    *\n");
		printf("*         %04d-%02d                                                    *\n",local->tm_year+1900,local->tm_mon+1);
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
		printf("\nusage:  %s [-i <input epgfile>] [-o <output epgfile>] [-v5|-v7|-v7be|-v7le|-xmltv ] [-bom] [-d|-dd] [-?|-h|--help]\n",basename);
		return 0;
	}

	epg e; 
	if(mode==2)e.debug=1;
	inputed=e.loadepg(infile);
	if(outfile != "")
		e.saveepg(outfile,wmode,bom);
	if(mode)
		e.dispepg();
	e.save_tvmap("tvmap.dat");
	return 0;
}

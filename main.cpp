#include "lib/epg.h"
#include "lib/estring.h"

#define BUILD_DATE	"2010年10月"
#define VERSION		"1.1.4"

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
#ifndef __WIN32__
		printf("**********************************************************************\n");
		printf("*         EPG数据转换器     v%s                                   *\n",VERSION);
		printf("*         作者：nx111	gdzdz@163.com                                *\n");
		printf("*         %s                                                  *\n",BUILD_DATE);
		printf("**********************************************************************\n");
#else
		printf(             "**********************************************************************\n");
		printf(UTF8ToGB2312("*         EPG数据转换器     v%s                                   *\n").c_str(),VERSION);
		printf(UTF8ToGB2312("*         作者：nx111	gdzdz@163.com                                *\n").c_str());
		printf(UTF8ToGB2312("*         %s                                                  *\n").c_str(),BUILD_DATE);
		printf(             "**********************************************************************\n");

#endif

		printf("\nusage:%s [-i <input epgfile>] [-o <output epgfile>] [-v5|-v7|-v7be|-v7le|-xmltv ] [-d|-dd] [-?|-h|--help]\n",argv[0]);
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

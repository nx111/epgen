/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "spdomparser.hpp"
#include "spdomiterator.hpp"
#include "spxmlnode.hpp"

enum { eXMLDOC, eDOCDECL, ePI, eDOCTYPE, eELEMENT, eCDATA, eCOMMENT  };
char *TYPENAME[]={"eXMLDOC", "eDOCDECL", "ePI", "eDOCTYPE", "eELEMENT", "eCDATA", "eCOMMENT"};
int main( int argc, char * argv[] )
{
	char * filename = NULL;

	if( argc < 2 ) {
#		ifdef WIN32
		filename = "..\\test.xml";
#		else
		printf( "Usage: %s <xml_file>\n", argv[0] );
		exit( -1 );
#		endif
	} else {
		filename = argv[1];
	}

	FILE * fp = fopen ( filename, "r" );
	if( NULL == fp ) {
		printf( "cannot not open %s\n", filename );
		exit( -1 );
	}

	struct stat aStat;
	char * source = NULL;
	stat( filename, &aStat );
	source = ( char * ) malloc ( aStat.st_size + 1 );
	fread ( source, aStat.st_size, sizeof ( char ), fp );
	fclose ( fp );
	source[ aStat.st_size ] = 0;

	SP_XmlDomParser parser;
	parser.append( source, strlen( source ) );
	free( source );


	SP_DomIterator iter( parser.getDocument() );
	for( const SP_XmlNode * node = iter.getNext();
			NULL != node;
			node = iter.getNext() ) {
		if(node->getType()==SP_XmlNode::eELEMENT){
			const char *iname=((SP_XmlElementNode*)node)->getName();
			if(strcmp(iname,"programme")==0){
				const char* schannelid=((SP_XmlElementNode*)node)->getAttrValue("channel");
				const char* sstart=((SP_XmlElementNode*)node)->getAttrValue("start");
				const char* sstop=((SP_XmlElementNode*)node)->getAttrValue("stop");
				
				const SP_XmlNodeList * children=((SP_XmlElementNode*)node)->getChildren();
				int childcount=children->getLength();
				printf("[%s]%6s %s %s \n", iname,schannelid,sstart,sstop );
				for(int j=0;j<childcount;j++){
					const char *childname=((SP_XmlElementNode*)(children->get(j)))->getName();
					const SP_XmlNodeList * tchild=((SP_XmlElementNode*)(children->get(j)))->getChildren();
					const char * text;
					if (tchild->getLength() && tchild->get(0)->getType()==SP_XmlNode::eCDATA)
						text=((SP_XmlCDataNode*)(tchild->get(0)))->getText();
					else 
						text="";
					printf("\t[%s]%s\n",childname,text );
					
				}
			}
			if(strcmp(iname,"channel")==0){
				const char* schannelid=((SP_XmlElementNode*)node)->getAttrValue("id");
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
					printf("<<%s>> id=%s\n",text,schannelid);	
				}			
			}
		}
	}

	if( NULL != parser.getError() ) {
		printf( "\n\nerror: %s\n", parser.getError() );
	}

#ifdef WIN32
	getchar();
#endif

	return 0;
}


/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "spxmlparser.hpp"
#include "spxmlevent.hpp"
#include "spxmlutils.hpp"
#include "spxmlcodec.hpp"

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
	source[ aStat.st_size ] = '\0';

	SP_XmlPullParser parser;
	parser.append( source, strlen( source ) );

	free( source );

	for( SP_XmlPullEvent * event = parser.getNext();
			NULL != event;
			event = parser.getNext() ) {
		switch( event->getEventType() ) {
			case SP_XmlPullEvent::eStartDocument:
				printf( "start document\n" );
				break;
			case SP_XmlPullEvent::eEndDocument:
				printf( "\nend document\n" );
				break;
			case SP_XmlPullEvent::eDocDecl:
				{
					SP_XmlDocDeclEvent * declEvent = (SP_XmlDocDeclEvent*)event;
					printf( "<?xml" );
					if( '\0' != *declEvent->getVersion() ) {
						printf( " version=\"%s\"", declEvent->getVersion() );
					}
					if( '\0' != *declEvent->getEncoding() ) {
						printf( " encoding=\"%s\"", declEvent->getEncoding() );
					}
					if( -1 != declEvent->getStandalone() ) {
						printf( " standalone=\"%s\"", declEvent->getStandalone() ? "yes" : "no" );
					}
					printf( "?>" );
					break;
				}
			case SP_XmlPullEvent::eDocType:
				{
					SP_XmlDocTypeEvent * typeEvent = (SP_XmlDocTypeEvent*)event;
					printf( "<!DOCTYPE %s PUBLIC \"%s\" SYSTEM \"%s\" \"%s\">",
							typeEvent->getName(), typeEvent->getPublicID(),
							typeEvent->getSystemID(), typeEvent->getDTD() );
					break;
				}
			case SP_XmlPullEvent::eStartTag:
				{
					SP_XmlStartTagEvent * stagEvent = (SP_XmlStartTagEvent*)event;
					printf( "\n<%s", stagEvent->getName() );
					for( int i = 0; i < stagEvent->getAttrCount(); i++ ) {
						const char * name = NULL, * value = NULL;
						name = stagEvent->getAttr( i, &value );
						printf( " %s=\"%s\"", name, value );
					}
					printf( ">" );
					break;
				}
			case SP_XmlPullEvent::eEndTag:
				printf( "</%s>", ((SP_XmlTextEvent*)event)->getText() );
				break;
			case SP_XmlPullEvent::eCData:
				{
					SP_XmlStringBuffer buffer;
					SP_XmlStringCodec::encode( parser.getEncoding(),
							((SP_XmlTextEvent*)event)->getText(), &buffer );
					printf( "%s", buffer.getBuffer() );
					break;
				}
			case SP_XmlPullEvent::eComment:
				printf( "<!--%s-->", ((SP_XmlTextEvent*)event)->getText() );
				break;
			case SP_XmlPIEvent::ePI:
				printf( "<?%s %s?>", ((SP_XmlPIEvent*)event)->getTarget(),
					((SP_XmlPIEvent*)event)->getData() );
				break;
		};

		delete event;
	}

	if( NULL != parser.getError() ) {
		printf( "\n\nerror: %s\n", parser.getError() );
	}

#ifdef WIN32
	getchar();
#endif

	return 0;
}


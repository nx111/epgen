/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "spdomparser.hpp"
#include "spxmlnode.hpp"
#include "spcanonxml.hpp"

int main( int argc, char * argv[] )
{
	if( argc < 3 ) {
		printf( "Usage: %s <in_file> <out_file>\n", argv[0] );
		exit( -1 );
	}

	char * source = NULL;
	{
		FILE * fpIn = fopen ( argv[1], "r" );
		if( NULL == fpIn ) {
			printf( "cannot not open input file: %s\n", argv[1] );
			exit( -1 );
		}

		struct stat aStat;

		stat( argv[1], &aStat );
		source = ( char * ) malloc ( aStat.st_size + 1 );
		fread ( source, aStat.st_size, sizeof ( char ), fpIn );
		fclose ( fpIn );
		source[ aStat.st_size ] = 0;
	}

	SP_XmlDomParser parser;
	parser.setIgnoreWhitespace( 0 );
	parser.append( source, strlen( source ) );

	free( source );

	if( NULL != parser.getError() ) {
		printf( "\n\nerror: %s\n", parser.getError() );
		exit( -1 );
	}

	SP_CanonXmlBuffer buffer( parser.getDocument() );
	{
		FILE * fpOut = fopen( argv[2], "w" );
		if( NULL == fpOut ) {
			printf( "cannot not open output file: %s\n", argv[2] );
			exit( -1 );
		}

		fwrite( buffer.getBuffer(), buffer.getSize(), sizeof( char ), fpOut );

		fclose( fpOut );
	}

	return 0;
}


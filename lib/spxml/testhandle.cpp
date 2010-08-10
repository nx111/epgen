/*
 * Copyright 2008 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include "spdomparser.hpp"
#include "spxmlnode.hpp"
#include "spxmlhandle.hpp"

int main( int argc, char * argv[] )
{
	const char * source = "<Document>"
		"<Element attributeA = \"valueA\">"
			"<Child attributeB = \"value1\" />"
			"<Child attributeB = \"value2\" />"
		"</Element>"
	"<Document>";

	SP_XmlDomParser parser;
	parser.append( source, strlen( source ) );

	if( NULL != parser.getError() ) {
		printf( "\n\nerror: %s\n", parser.getError() );
	}

	SP_XmlHandle rootHandle( parser.getDocument()->getRootElement() );

	SP_XmlElementNode * child2 = rootHandle.getChild( "Element" )
			.getChild( "Child", 1 ).toElement();

	if( NULL != child2 ) {
		printf( "Find Document/Element/Child[2]\n" );
	} else {
		printf( "Cannot found Document/Element/Child[2]\n" );
	}

	SP_XmlElementNode * child1 = rootHandle.getElement(0).getElement(0).toElement();

	if( NULL != child1 ) {
		printf( "Find Document/Element/Child[1]\n" );
	} else {
		printf( "Cannot found Document/Element/Child[1]\n" );
	}

	printf( "\n" );

#ifdef WIN32
	getchar();
#endif

	return 0;
}


CC = gcc
CFLAGS = -Wno-deprecated -D_REENTRANT -D_GNU_SOURCE -fPIC
LDFLAGS = -lstdc++

LINKER = $(CC)
RM = /bin/rm -f


vpath %   lib/spxml
vpath %   lib/libmd5sum
vpath %   lib

OBJS=endian.o estring.o dvb.o epg.o 
OBJS_XML=spxmlutils.o spxmlevent.o spxmlreader.o spxmlparser.o spxmlstag.o \
        spxmlnode.o spdomparser.o spdomiterator.o spxmlcodec.o spxmlhandle.o
OBJS_MD5=libmd5sum.o md5.o
XML_TEST=testpull testdom testxmlconf testhandle
#----------------------------------------------------------

default: all_with_xml

all_no_xml: base main md5sum
	$(LINKER) $(LDFLAGS) $(patsubst %,obj/%,$(OBJS)) $(patsubst %,obj/%,$(OBJS_MD5)) obj/main.o -o epgen 

all_with_xml : base spxml main md5sum
	$(LINKER) $(LDFLAGS) $(patsubst %,obj/%,$(OBJS))  $(patsubst %,obj/%,$(OBJS_MD5)) $(patsubst %,obj/%,$(OBJS_XML)) obj/main.o -o epgen 

main : main.o
	
test: .test
	$(LINKER) $(LDFLAGS) obj/estring.o obj/test.o -o test
.test: test.o estring.o

base : endian.o estring.o dvb.o epg.o

md5sum:libmd5sum.o md5.o

spxml : spxmlutils.o spxmlevent.o spxmlreader.o spxmlparser.o spxmlstag.o \
        spxmlnode.o spdomparser.o spdomiterator.o spxmlcodec.o spxmlhandle.o



xmltest:$(XML_TEST)

testpull: testpull.o spxml
	$(LINKER) $(LDFLAGS) obj/testpull.o  $(patsubst %,obj/%,$(OBJS_XML)) -o testpull

testdom: testdom.o spxml
	$(LINKER) $(LDFLAGS) obj/testdom.o  $(patsubst %,obj/%,$(OBJS_XML)) -o testdom

testxmlconf: testxmlconf.o spcanonxml.o spxml
	$(LINKER) $(LDFLAGS) obj/testxmlconf.o obj/spcanonxml.o  $(patsubst %,obj/%,$(OBJS_XML)) -o testxmlconf

testhandle: testhandle.o spxml
	$(LINKER) $(LDFLAGS) obj/testhandle.o  $(patsubst %,obj/%,$(OBJS_XML)) -o testhandle


	
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o obj/$@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o obj/$@	
clean:
	rm -f *.o obj/*.o

COMPILER=gcc
FILES=tg.c
FLAGSC= -g3 -fpic -c
NAMELIB=libtg
LIBS=-lssl -lcrypto

all: compilelib linkinglib

compilelib:
	$(COMPILER) $(FILES) $(FLAGSC) -o $(NAMELIB).o $(LIBS)
linkinglib:
	$(COMPILER) -Wl,-export-dynamic -shared $(NAMELIB).o -o $(NAMELIB).so $(LIBS)

clean:
	rm $(NAMELIB).o
	rm $(NAMELIB).so
	rm test
	
test:
	gcc test.c -ltg -g3 -L./ -o test

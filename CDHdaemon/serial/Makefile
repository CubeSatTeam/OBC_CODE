#dependency objects on simpleDataLink library
depobj=simpleDataLink/build/simpleDataLink.o \
simpleDataLink/build/bufferUtils.o \
simpleDataLink/build/frameUtils.o

#dependency objects headers
depinc=-I simpleDataLink/inc/ \
-I simpleDataLink/lib/bufferUtils/inc/ \
-I simpleDataLink/lib/frameUtils/inc/

serialInterface.so: serialInterface.o $(depobj)
	$(CC) -Wall -shared -o $@ serialInterface.o $(depobj)
	rm serialInterface.o
	
serialInterface.o: serialInterface.c
	$(CC) -Wall -fPIC -o $@ -c serialInterface.c $(depinc)

.PHONY: $(depobj)
$(depobj): 
	cd simpleDataLink && make compflags="-Wall -fPIC"

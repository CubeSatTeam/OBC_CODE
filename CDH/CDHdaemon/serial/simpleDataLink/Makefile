#sources
sources=src/simpleDataLink.c \
lib/bufferUtils/src/bufferUtils.c \
lib/frameUtils/src/frameUtils.c
vpath %.c $(dir $(sources))

#objects
objects=$(addprefix $(builddir)/,$(notdir $(sources:.c=.o)))

#include paths
includes=-Iinc/ \
-Ilib/bufferUtils/inc/ \
-Ilib/frameUtils/inc/
vpath %.h $(includes:-I%=%)

#output directory
builddir=build
#compiler flags
compflags=-Wall

$(builddir)/simpleDataLink.a: $(objects) | $(builddir)
	$(AR) rcs $(builddir)/simpleDataLink.a $(objects)

$(builddir)/%.o: %.c %.h | $(builddir)
	$(CC) $(compflags) -o $@ -c $< $(includes)

example: examples/communicationExample.c $(builddir)/simpleDataLink.a | $(builddir)
	$(CC) $(compflags) -o $(builddir)/communicationExample.o -c $< $(includes)
	$(CC) -o $(builddir)/communicationExample $(builddir)/communicationExample.o $(builddir)/simpleDataLink.a

$(builddir):
	mkdir $@

.PHONY: clean
clean:
	rm -r $(builddir)
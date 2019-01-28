##################################
# Peter Thompson March 2016
# pmtfxosd makefile
# based on helloworld makefile ~/Documents/maketemplate/makeDEVELOP/genericmake/
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# https://www.duke.edu/cps108/doc/makefileinfo/sample.html
# see makefiles booklet prepared by peter
# google "typical makefile example"
# note - a tab goes at the beginning of each make command line
#########################################
# and don't forget the 3 stooges... only 1st one needed here...
#  export PATH=$PATH:/usr/local/xtools/arm-unknown-linux-gnueabi/bin/
#  export ARCH=arm
#  export CROSS_COMPILE=arm-linux-
#
##############################################

# hello application ==> 2 lines to change
SOURCES = fxos8700stub.c  pmtfxosdaemon.c  getdieface.c  # list of 3 source files
EXECUTABLE = pmtfxosd         # 2nd of 2 lines to change

# hello directories
SRCDIR = ./src
OBJDIR = ./obj

# compiler and linker flags to use 
CC = gcc

CFLAGS += -I$(SRCDIR)/include/ 
LDFLAGS =  # -L  directory location of libraries
LDLIBS += -lm -lrt # -lSDL -lm ... all libraries linked in
STATIC = # -static # for static (not dynamic) link 

# create list of object filenames *.o from source filenames *.c and print them
XOBJECTS = $(SOURCES:.c=.o)  
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(XOBJECTS))  # prefix with obj/ directory
$(warning OBJECTS is $(OBJECTS))     # print list of object files for debu


###################################################
# This is the crux of the make program
##############################################


#  objective is to make EXECUTABLE
all: $(EXECUTABLE)     

# gcc 8sourcefiles.o -o pmtfxosd
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(STATIC) $(LDFLAGS)  $(OBJECTS) $(LDLIBS) -o $@

# gcc -c helloworld.c -o helloworld.o   NOTE .c.o: is a convention
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@


##########################################


clean:
	rm $(OBJDIR)/*.o; rm pmtfxosd; rm $(SRCDIR)/*~; rm *~






##################################
# Peter Thompson March 2016
# pmtdiefaced makefile
# based on helloworld makefile ~/Documents/maketemplate/makeDEVELOP/genericmake/
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# https://www.duke.edu/cps108/doc/makefileinfo/sample.html
# see makefiles booklet prepared by peter
# google "typical makefile example"
# note - a tab goes at the beginning of each make command line
#########################################
# This makefile works for X86 or ARM by changing CC variable
#
##############################################

# hello application ==> 2 lines to change
SOURCES =  diefacehandler.c diefacemain.c pmtdiefacedaemon.c  # list of 3 source files
EXECUTABLE = pmtdiefaced         # 2nd of 2 lines to change

# hello directories
SRCDIR = ./src
OBJDIR = ./obj

# compiler and linker flags to use 
# CC = arm-linux-gcc
CC = gcc

CFLAGS += -I$(SRCDIR)/include/   -I../pmtfxosd/src/include/


LDFLAGS =  # -L  directory location of libraries
LDLIBS = -lrt  # -lSDL -lm ... all libraries linked in
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
	rm $(OBJDIR)/*.o; rm pmtdiefaced; rm $(SRCDIR)/*~; rm *~






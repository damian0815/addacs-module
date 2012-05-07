
SOURCES = $(shell echo *.c)
OBJDIR = obj
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(SOURCES:.c=.o))

INCLUDES=-I..
CFLAGS+=$(INCLUDES) -std=c99 -Wall -Os 
LDFLAGS+=-lrt
TARGET=$(shell basename `pwd`)

all: $(TARGET)

clean: 
	echo $(TARGET)
	rm $(OBJECTS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -o $@ $<
#%.o: %.c 
#	$(CC) -c $(CFLAGS) -c -o $@ $<



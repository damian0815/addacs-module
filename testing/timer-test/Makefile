
SOURCES = $(shell echo *.c)
OBJDIR = obj
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(SOURCES:.c=.o))

INCLUDES=-Isrc -I../..
CFLAGS+=$(INCLUDES) -std=gnu99 -Wall -Os  -march=native 
LDFLAGS+=-lrt
TARGET=$(shell basename `pwd`)

all: $(TARGET)

clean: 
	rm $(OBJECTS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -o $@ $<
#%.o: %.c 
#	$(CC) -c $(CFLAGS) -c -o $@ $<




SOURCES = $(shell echo src/*.c)
OBJECTS = $(SOURCES:.c=.o)

INCLUDES=-Isrc -I..
CFLAGS+=$(INCLUDES) -std=c99 -Wall -Os 
TARGET=daemon

all: daemon

clean: 
	rm $(OBJECTS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

#%.o: %.c 
#	$(CC) -c $(CFLAGS) -c -o $@ $<



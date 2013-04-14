## Makefile for ENGR421 Sp2013 crossfire robot

TARGET = edge_detect

SOURCES = $(TARGET).c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv` -lpthread


all: $(TARGET)

$(TARGET) : $(OBJECTS)
	g++ $(CFLAGS) -o $@ $< $(LIBS)

%.o : %.cpp
	g++ $(CFLAGS) -c -o $@ $< $(LIBS)

clean:
	rm -rf $(OBJECTS)
	rm -rf $(TARGET)

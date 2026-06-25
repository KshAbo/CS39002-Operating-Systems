CC = gcc
CFLAGS = -Wall -pthread
TARGET = manager

all: $(TARGET)

$(TARGET): manager.c worker.c dlhandler.c global.h
	$(CC) $(CFLAGS) -o $(TARGET) manager.c

run: $(TARGET)
	./$(TARGET) 5 15 25

demorun: $(TARGET)
	./$(TARGET) 8 16 32

clean:
	rm -f $(TARGET)

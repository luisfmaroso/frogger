CC      = gcc
CFLAGS  = -Wall -Wextra -O2
TARGET   = frogger.exe

all: $(TARGET)

$(TARGET): frogger.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all run clean

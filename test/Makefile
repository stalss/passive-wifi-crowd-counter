# Native test harness (no ESP-IDF needed)
# Run: make test

CC = gcc
CFLAGS = -I./mock_esp -I../main -Wall -Wextra -O2
TARGET = test_native

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): test_native.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f $(TARGET)

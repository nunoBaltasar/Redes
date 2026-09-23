CC     = g++
CFLAGS = -Wall -Wextra -std=c++11

all: user

user: user.cpp protocol.cpp protocol.h
	$(CC) $(CFLAGS) -o user user.cpp protocol.cpp

clean:
	rm -f user

.PHONY: all clean

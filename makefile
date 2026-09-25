CC     = g++
CFLAGS = -Wall -Wextra -std=c++11

all: user

user: user.cpp clientUDP.cpp clientUDP.h validation.cpp validation.h
	$(CC) $(CFLAGS) -o user user.cpp clientUDP.cpp validation.cpp

clean:
	rm -f user

.PHONY: all clean

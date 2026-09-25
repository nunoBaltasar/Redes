CC     = g++
CFLAGS = -Wall -Wextra -std=c++11

all: user

user: user.cpp clientUDP.cpp clientUDP.h
	$(CC) $(CFLAGS) -o user user.cpp clientUDP.cpp

clean:
	rm -f user

.PHONY: all clean

user: user.cpp
	g++ user.cpp -o user

clean:
	re -f user

.PHONY: clean
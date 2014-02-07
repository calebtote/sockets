TARGETS=client server

CC=g++
CCOPTS=-Wall -Wextra -g

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.cpp
	$(CC) $(CCOPTS) -o $@ $<
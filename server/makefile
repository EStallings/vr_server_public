# makefile for macosx

flags = -Wall -DDEBUG # -O3 

% : %.cpp Net.h Model.h
	clang++ -std=c++11 $< -o $@ ${flags}

all : Server

server : Server
	./Server

clean:
	rm -f Server

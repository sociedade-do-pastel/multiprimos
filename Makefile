all: proc2 proc3

proc2: proc2.o sock.o
	g++ sock.o $< -o $@

proc2.o: proc2.cpp sock.hpp

proc3: proc3.o sock.o
	g++ sock.o $< -o $@ -lpthread

proc3.o: proc3.cpp sock.hpp

sock.o : sock.hpp

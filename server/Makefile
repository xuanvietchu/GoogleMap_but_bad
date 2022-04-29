all: server

server: digraph.o server.o dijkstra.o
	g++ digraph.o server.o dijkstra.o -o server

dijkstra.o: dijkstra.cpp dijkstra.h
	g++ -c dijkstra.cpp

digraph.o: digraph.cpp digraph.h 
	g++ -c digraph.cpp

server.o: server.cpp wdigraph.h
	g++ -c server.cpp

clean: 
	rm *.o server
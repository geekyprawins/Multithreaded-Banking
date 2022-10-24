	CC=gcc

FLAGS = -pthread -Wall -pedantic
#pthread: Link to threaded library
# pedantic: it warns if you are performing OS specific optimisation
# Wall all warnings turned on
ALL:client server

client: client.o 
	$(CC) $(FLAGS) client.o -o client 
server: server.o
	$(CC) $(FLAGS) server.o -o server

server.o: server.c
	$(CC) $(FLAGS) -c server.c 

client.o: client.c 
	$(CC) $(FLAGS) -c client.c

clean:
	rm -rf *.o client server
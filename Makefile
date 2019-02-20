make: 	client.c server.c
	gcc -o server server.c -lpthread
	gcc -o client client.c

clean: 
	rm -f *.exe
	rm -f *.o
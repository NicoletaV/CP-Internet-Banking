build:
	g++ -std=c++11 -Wall -g -lnsl client.cpp -o client
	g++ -std=c++11 -Wall -g -lnsl server.cpp -o server

clean:
	rm -f client server
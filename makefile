release:
	gcc *.c -lpthread -Wall -O2 -o main

debug:
	gcc *.c -lpthread -Wall -g -D _DEBUG -o main
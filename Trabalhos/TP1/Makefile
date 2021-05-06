all:
	gcc -g -Wall -c funcoes.c
	gcc -g -Wall cliente.c funcoes.o -o cliente
	gcc -g -Wall servidor.c funcoes.o -o servidor

clean:
	rm funcoes.o cliente servidor
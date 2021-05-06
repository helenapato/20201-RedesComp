all:
	gcc -g -Wall -c funcoes.c
	gcc -pthread -g -Wall servidor_dns.c funcoes.o -o servidor_dns

clean:
	rm funcoes.o servidor_dns
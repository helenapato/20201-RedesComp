#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <arpa/inet.h>

void logexit(const char *msg) 
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int client_init(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) 
// Faz o parsing do cliente, retorna -1: erro, e 0: funcionou
{
    // Parametros nulos
    if (addrstr == NULL || portstr == NULL) return -1;

    // Faz o cast do porto e coloca em network byte order
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0) return -1;
    port = htons(port);

    // Testa se o endereco e IPv4
    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    // Testa se o endereco e IPv6
    struct in6_addr inaddr6;
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

int server_init(const char *portstr, struct sockaddr_storage *storage) 
// Faz o parsing do servidor, retorna -1: erro, e 0: funcionou
{
    // Faz o cast do porto e coloca em network byte order
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0) return -1;
    port = htons(port);

    // Inicializa storage
    memset(storage, 0, sizeof(*storage));

    // Constante que define a versao. 0: IPv4, 1: IPv6
    int versao = 1;

    // Se e IPv4
    if (versao == 0) 
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    // Se e IPv6
    else if (versao == 1) 
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    // Erro
    else return -1;
}
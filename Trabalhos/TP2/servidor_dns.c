#include "funcoes.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define MAX_ENTRADAS 1000 // Numero maximo de entradas nas tabelas
#define TAM1 100 // Tamanho das entradas das tabelas
#define TAM2 10 // Tamanho das entradas das tabelas

// Cria arrays para hostnames e links
char hostname_table [MAX_ENTRADAS][TAM1];
char hostip_table [MAX_ENTRADAS][TAM1];
int n_hosts = 0;
char linkip_table [MAX_ENTRADAS][TAM1];
char linkport_table [MAX_ENTRADAS][TAM2];
int n_links = 0;


int busca_hostname(char* hostname, char* ip_retorno)
{
    // Procura o hostname localmente
    int i;
    for(i=0;i<n_hosts;i++)
    {
        if(strcmp(hostname_table[i], hostname) == 0)
        {
            strcpy(ip_retorno, hostip_table[i]);
            return 1;
        }
    }
    
    // Procura o hostname nos servidores ligados, caso existam
    for(i=0;i<n_links;i++)
    {
        // Cria storage
        struct sockaddr_storage storage;
        int erro = client_init(linkip_table[i], linkport_table[i], &storage);
        if(erro != 0) logexit("storage_cliente");

        // Cria socket
        int client_socket = socket(storage.ss_family, SOCK_DGRAM, 0);
        if(client_socket == -1) logexit("socket_cliente");

        // Cria um sockaddr
        struct sockaddr *addr = (struct sockaddr *)(&storage);
        socklen_t addr_len = sizeof(storage);

        // Coloca o hostname no buffer
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        buf[0] = '1';
        int j;
        for(j=0;j<strlen(hostname);j++)
        {
            buf[j+1] = hostname[j];
        }

        // Timeout
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) logexit("setsockopt_timeout");

        // Envia requisicao
        ssize_t count = sendto(client_socket, buf, strlen(hostname) + 1, MSG_CONFIRM, addr, addr_len);
        if (count != strlen(hostname) + 1) logexit("envio_requisicao");
        
        // Recebe resposta
        memset(buf, 0, BUFSZ);
        count = recvfrom(client_socket, buf, BUFSZ, MSG_WAITALL, addr, &addr_len);
        if(count == 0) logexit("recebimento_resposta");
        int t = 0;
        if(buf[0] != '2') t = 1;//logexit("protocolo_resposta");

        // Se a resposta foi positiva
        if(t == 0 && buf[1] != '-')
        {
            char* ip_encontrado = &buf[1];
            strcpy(ip_retorno, ip_encontrado);
            return 1;
        }
        close(client_socket);
    }

    // Resposta negativa em ultimo caso
    return 0;
}


void* aguarda_conexao(void* param)
{
    // Cria storage
    struct sockaddr_storage storage;
    int erro = server_init(param, &storage);
    if(erro != 0) logexit("storage_server");

    // Cria socket do servidor
    int server_socket = socket(storage.ss_family, SOCK_DGRAM, 0);
    if(server_socket == -1) logexit("socket_server");

    // Opcoes do socket
    int enable = 1;
    erro = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if(erro != 0) logexit("setsockopt");

    // Atribui uma porta ao servidor
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    erro = bind(server_socket, addr, sizeof(storage));
    if(erro != 0) logexit("bind");

    // Loop para conexoes
    while(1)
    {
        // Cria um storage e um sockaddr para o outro servidor
        struct sockaddr_storage server2_storage;
        struct sockaddr *server2_addr = (struct sockaddr *)(&server2_storage);
        socklen_t server2_addr_len = sizeof(server2_storage);

        // Recebe a requisicao
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        ssize_t count = recvfrom(server_socket, buf, BUFSZ - 1, MSG_WAITALL, server2_addr, &server2_addr_len);
        if(count == 0) logexit("recvfrom_servidor");
        if(buf[0] != '1') logexit("protocolo_receber_requisicao");

        // Coloca o hostname recebido em uma variavel propria
        char* hostname = &buf[1];

        // Busca endereco
        char ip_retorno [TAM1];
        int resultado = busca_hostname(hostname, ip_retorno);
        
        // Envia resposta
        memset(buf, 0, BUFSZ);
        buf[0] = '2';
        if(resultado == 0) // Nao encontrado
        {
            buf[1] = '-';
            buf[2] = '1';
            count = sendto(server_socket, buf, 3, MSG_CONFIRM, server2_addr, server2_addr_len);
            if(count != 3) logexit("sendto_server_n");
        }
        if(resultado == 1) // Encontrado
        {
            int tam_ip = strlen(ip_retorno);
            int i;
            for(i=0;i<tam_ip;i++) buf[i+1] = ip_retorno[i];
            count = sendto(server_socket, buf, tam_ip + 1, MSG_CONFIRM, server2_addr, server2_addr_len);
            if (count != tam_ip + 1) logexit("sendto_server_p");
        }
    }
    pthread_exit(0);
}


int main(int argc, char **argv)
{
    // Numero insuficiente de argumentos
    if(argc < 2) logexit("argc_servidor");

    // Cria thread para esperar conexoes de outros servidores
    pthread_t tid; 
    char endereco [TAM1];
    strcpy(endereco, argv[1]);
    int erro = pthread_create(&tid, NULL, aguarda_conexao, (void *)endereco);
    if(erro != 0) logexit("pthread_create");

    // Loop para ler os comandos
    char comando [TAM2]; 
    char enter;
    char hostname [TAM1];
    char ip [TAM1];
    char port [TAM2];

    // Se nao tem arquivo de entrada
    if(argc == 2)
    {
        while(1)
        {
            // Le o comando
            scanf("%s", comando);
            
            // Comando add
            if(strcmp(comando, "add") == 0)
            {
                scanf("%s", hostname);
                scanf("%s", ip);
                scanf("%c", &enter);
                
                // Verifica se o hostname ja existe
                int i; 
                int atualizado = 0;
                for(i=0;i<n_hosts;i++)
                {
                    if(strcmp(hostname, hostname_table[i]) == 0)
                    {
                        // Se ele existe so atualiza o endereco
                        strcpy(hostip_table[i], ip);
                        atualizado = 1;
                        printf("Atualizado %s %s\n", hostname_table[i], hostip_table[i]);
                        break;
                    }
                }

                // Se ele nao existe insere no array
                if(atualizado == 0)
                {
                    strcpy(hostname_table[n_hosts], hostname);
                    strcpy(hostip_table[n_hosts], ip);
                    printf("Inserido %s %s\n", hostname_table[n_hosts], hostip_table[n_hosts]);
                    n_hosts++;
                }
            }

            // Comando search
            else if(strcmp(comando, "search") == 0)
            {
                scanf("%s", hostname);
                scanf("%c", &enter);

                // Chama funcao pra procurar o hostname
                char ip_retorno [TAM1];
                int resultado = busca_hostname(hostname, ip_retorno);
                if(resultado == 0) printf("Hostname %s nao encontrado\n", hostname);
                if(resultado == 1) printf("Encontrado %s %s\n", hostname, ip_retorno);
            }

            // Comando link
            else if(strcmp(comando, "link") == 0)
            {
                scanf("%s", ip);
                scanf("%s", port);
                scanf("%c", &enter);
                
                // Insere informacoes na tabela
                strcpy(linkip_table[n_links], ip);
                strcpy(linkport_table[n_links], port);
                printf("Ligado %s %s\n", linkip_table[n_links], linkport_table[n_links]);
                n_links++;
            }
            else printf("Comando %s invalido\n", comando);
        }
    }

    // Se tem arquivo de entrada
    if(argc == 3)
    {
        // Abre o arquivo
        FILE *startup = fopen(argv[2], "r");
        int fim;
        while(1)
        {
            // Le o comando e verifica o EOF
            fim = fscanf(startup, "%s", comando);
            if(fim == EOF) break;

            // Comando add
            if(strcmp(comando, "add") == 0)
            {
                fscanf(startup, "%s", hostname);
                fscanf(startup, "%s", ip);
                fscanf(startup, "%c", &enter);

                // Verifica se o hostname ja existe
                int i; 
                int atualizado = 0;
                for(i=0;i<n_hosts;i++)
                {
                    if(strcmp(hostname, hostname_table[i]) == 0)
                    {
                        // Se ele existe so atualiza o endereco
                        strcpy(hostip_table[i], ip);
                        atualizado = 1;
                        printf("Atualizado %s %s\n", hostname_table[i], hostip_table[i]);
                        break;
                    }
                }

                // Se ele nao existe insere no array
                if(atualizado == 0)
                {
                    strcpy(hostname_table[n_hosts], hostname);
                    strcpy(hostip_table[n_hosts], ip);
                    printf("Inserido %s %s\n", hostname_table[n_hosts], hostip_table[n_hosts]);
                    n_hosts++;
                }
            }

            // Comando search
            else if(strcmp(comando, "search") == 0)
            {
                fscanf(startup, "%s", hostname);
                fscanf(startup, "%c", &enter);

                // Chama funcao pra procurar o hostname
                char ip_retorno [TAM1];
                int resultado = busca_hostname(hostname, ip_retorno);
                if(resultado == 0) printf("Hostname %s nao encontrado\n", hostname);
                if(resultado == 1) printf("Encontrado %s %s\n", hostname, ip_retorno);
            }

            // Comando link
            else if(strcmp(comando, "link") == 0)
            {
                fscanf(startup, "%s", ip);
                fscanf(startup, "%s", port);
                fscanf(startup, "%c", &enter);
                
                // Insere informacoes na tabela
                strcpy(linkip_table[n_links], ip);
                strcpy(linkport_table[n_links], port);
                printf("Ligado %s %s\n", linkip_table[n_links], linkport_table[n_links]);
                n_links++;
            }
            else printf("Comando %s invalido\n", comando);
        }

        // Fecha o arquivo
        fclose(startup);
    }
}
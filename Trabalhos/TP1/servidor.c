#include "funcoes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define MAX_PALAVRA 127

// Recebe a porta para aguardar conexoes do cliente
int main(int argc, char **argv)
{
    // Numero insuficiente de argumentos
    if(argc < 2) logexit("argc_servidor");

    // Cria storage
    struct sockaddr_storage storage;
    int erro;
    erro = server_init(argv[1], &storage);
	if(erro != 0) logexit("storage_server");

    // Cria socket do servidor
    int server_socket;
    server_socket = socket(storage.ss_family, SOCK_STREAM, 0);
    if(server_socket == -1) logexit("socket_server");

    // Opcoes do socket
    int enable = 1;
    erro = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if(erro != 0) logexit("setsockopt");

    // Atribui uma porta ao servidor
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    erro = bind(server_socket, addr, sizeof(storage));
    if(erro != 0) logexit("bind");

    // Espera conexões
    erro = listen(server_socket, 10);
    if(erro != 0) logexit("listen");

    // Loop do funcionamento do servidor
    while(1)
    {
        // Cria um storage e um socket para o cliente
        struct sockaddr_storage client_storage;
        struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);
        socklen_t client_addr_len = sizeof(client_storage);

        // Aceita conexao
        int client_socket = accept(server_socket, client_addr, &client_addr_len);
        if (client_socket == -1) logexit("accept");

        // Inicio do jogo
        char palavra [MAX_PALAVRA];
        int tamanho_palavra = gera_palavra(palavra);

        // Envia mensagem de inicio de jogo ao ciente
        char inicio [2] = {1, tamanho_palavra};
        size_t count = send(client_socket, inicio, 2, 0);
        if (count != 2) logexit("send_server");

        // Loop para receber os palpites
        while(1)
        {
            // Recebe o palpite
            char buf[2];
            memset(buf, 0, 2);
            count = recv(client_socket, buf, 3, 0);
            if(buf[0] != 2) logexit("palpite_servidor");
            char palpite = buf[1];

            // Checa se o palpite esta na palavra
            int i; 
            int n_ocorrencias = 0;
            for(i=0;i<tamanho_palavra;i++)
            {
                if(palpite == palavra[i]) n_ocorrencias++;
            }
            // Palpite nao esta na palavra
            if(n_ocorrencias == 0)
            {
                char resposta [2] = {3, 0};

                count = send(client_socket, resposta, 2, 0);
                if (count != 2) logexit("server_resposta_negativa");
            }
            // Palpite esta na palavra
            else
            {
                // Checa as posicoes que o palpite ocorre
                char posicoes [n_ocorrencias];
                int acertos = 0; 
                int j = 0;
                for(i=0;i<tamanho_palavra;i++)
                {
                    if(palavra[i] == palpite)
                    {
                        posicoes[j] = i;
                        palavra[i] = '*';
                        j++;
                    }
                    if(palavra[i] == '*') acertos++;
                }
                // Fim de jogo, todas as letras ja foram acertadas
                if(acertos == tamanho_palavra)
                {
                    char resposta [1] = {4};

                    count = send(client_socket, resposta, 1, 0);
                    if (count != 1) logexit("server_resposta_fim");
                    close(client_socket);
                    break;
                }
                // Envia ocorrencias e posicoes
                else
                {
                    char resposta [n_ocorrencias+2];
                    resposta[0] = 3;
                    resposta[1] = n_ocorrencias;
                    for(i=0;i<n_ocorrencias;i++) resposta[i+2] = posicoes[i];
                    
                    count = send(client_socket, resposta, n_ocorrencias+2, 0);
                    if (count != n_ocorrencias+2) logexit("server_resposta_posicoes");
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}
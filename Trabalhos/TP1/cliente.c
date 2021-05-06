#include "funcoes.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_PALAVRA 127

// Recebe o endereço IP e a porta do servidor
int main(int argc, char **argv)
{
    // Numero insuficiente de argumentos
    if(argc < 3) logexit("argc_cliente");

    // Cria storage
    struct sockaddr_storage storage;
    int erro;
    erro = client_init(argv[1], argv[2], &storage);
	if(erro != 0) logexit("storage_cliente");

    // Cria socket
	int client_socket;
	client_socket = socket(storage.ss_family, SOCK_STREAM, 0);
	if(client_socket == -1) logexit("socket_cliente");

    // Conecta ao servidor
	struct sockaddr *addr = (struct sockaddr *)(&storage);
    erro = connect(client_socket, addr, sizeof(storage));
	if(erro != 0) logexit("connect_cliente");

    // Recebe mensagem inicio jogo do servidor
    char buf[MAX_PALAVRA+2];
    memset(buf, 0, 2);
    size_t count = recv(client_socket, buf, 3, 0);
	if (count == 0) logexit("inicio_cliente");
    if (buf[0] != 1) logexit("tipo_inicio_cliente");

    // Guardando as informacoes sobre o jogo
    int tamanho_palavra = buf[1];
    char palavra [tamanho_palavra];
    printf("\nJogo de forca\nTema: frutas");
    printf("\nO tamanho da palavra é %d\n", tamanho_palavra);
    int i;
    for(i=0;i<tamanho_palavra;i++)
    {
        palavra[i] = '_';
        printf(" %c ", palavra[i]);
    }

    // Loop para os palpites
    while(1)
    {
        // Envia palpite
        char letra; char enter;
        printf("\nEnvie seu palpite: ");
        scanf("%c", &letra);
        scanf("%c", &enter);
        char palpite [2] = {2, letra};
        count = send(client_socket, palpite, 2, 0);
        if (count == 0) logexit("envio_palpite");
        
        // Recebe resposta
        memset(buf, 0, MAX_PALAVRA+2);
        count = recv(client_socket, buf, MAX_PALAVRA+2, 0);
        if (count == 0) logexit("recebimento_posicoes");

        // Verifica se e fim de jogo
        if(buf[0] == 4)
        {
            printf("\nFim de jogo, voce acertou a palavra:\n");
            for(i=0;i<tamanho_palavra;i++)
            {
                if(palavra[i] == '_') printf("%c", letra);
                else printf("%c", palavra[i]);
            }
            printf("\n");
            break;
        }
        if(buf[0] != 3) logexit("resposta_servidor_cliente");

        /* Nao e fim de jogo, verifica numero de ocorrencias 
           da letra na palavra */
        int n_ocorrencias = buf[1];
        // Letra nao ocorre na palavra
        if(n_ocorrencias == 0)printf("\nEssa letra nao aparece na palavra");
        // Letra ocorre na palavra
        else
        {
            printf("\nEssa letra aparece na palavra:\n");
            // Preenche lacunas na palavra
            for(i=0;i<n_ocorrencias;i++) palavra[(int)buf[i+2]] = letra;
            // Imprime a palavra
            for(i=0;i<tamanho_palavra;i++) printf(" %c ",palavra[i]);
        }
    }
    close(client_socket);
    exit(EXIT_SUCCESS);
}
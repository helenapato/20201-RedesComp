#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

void logexit(const char *msg);
int client_init(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
int server_init(const char *portstr, struct sockaddr_storage *storage);
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

enum serv {
    SERV_QOTD
};

int main (
    int argc,
    char *argv[]
){
    enum serv service;
    struct in_addr addr;
    struct servent *qotd_servent;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Uso: %s <direccion IP> [-s servicio]\n", argv[0]);
        exit(1);
    }

    if (argc == 3) {
        fprintf(stderr, "Uso: %s <direccion IP> [-s servicio]\n", argv[0]);
        exit(1);
    }

    //TODO: Usar strcmp para comparar cadenas en lugar de '=='
    if (argc == 3 && argv[3] == "-s") {
        //TODO: Usar strcmp para comparar cadenas en lugar de '=='
        if (argv[4] == "qotd") {
            service = SERV_QOTD;
        } else {
            fprintf(stderr, "Servicio no soportado: %s\n", argv[4]);
            exit(1);
        }
    } else {
        service = SERV_QOTD; // Default service
    }

    //TODO: Validar la dirección IP
    inet_aton(argv[1], &addr);
    //TODO: Validar el puerto del servicio
    getservbyname("qotd", "udp")->s_port;

    //TODO: Implementar la lógica del cliente QOTD aquí

    return 0;
}

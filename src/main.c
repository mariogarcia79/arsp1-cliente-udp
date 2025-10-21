/*
    * main.c
    *
    *  Created on: 12 may. 2024
    *
    * GENERAL TODO LIST:
    * - Treat errors with defined signals
    * - Use perror() and errno functions, maybe define my own perror wrappers
    * - Wrap long code blocks into functions
    * - Make code reusable and modular
    * - Add comments and documentation
    * - Use constants instead of literals
    * - Improve argument parsing
    * - Implement logging mechanism
    * - Add configuration file support
    * - Implement unit tests
    * - Optimize performance
    * - Ensure code adheres to one C standard (Look for Slackware)
    * - Clean imports
    * - Follow consistent coding style
    * - Validate all user inputs
    * - Handle edge cases
    * - Refactor for readability
    * - Add \n at the end of error prints
    * - Use getaddrinfo() instead of manual sockaddr_in setup ***
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>

int main (
    int argc,
    char *argv[]
){
    // Implementar enum service para soportar multiples servicios
    char *proto = "udp";
    char *service;
    struct sockaddr_in addr;
    struct servent *qotd_servent;
    int sockfd;
    // Mirar si hay que hacer reserva dinamica o el mensaje tiene un limite.
    // Mejor reserva dinamica.
    char msg[512];
    struct sockaddr_in myaddr;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Uso: %s <direccion IP> [-s servicio]\n", argv[0]);
        exit(1);
    }

    if (argc == 3) {
        fprintf(stderr, "Uso: %s <direccion IP> [-s servicio]\n", argv[0]);
        exit(1);
    }

    if (argc == 3 && !strcmp(argv[3], "-s")) {
        if (!strcmp(argv[4], "qotd"))
            service = "qotd";
        else {
            fprintf(stderr, "Servicio no soportado: %s\n", argv[4]);
            exit(1);
        }
    } else {
        // Servicio por defecto
        service = "qotd";
    }

    //TODO: Crear constante UDP en lugar del literal, permitir TCP a futuro.
    if ((qotd_servent = getservbyname("qotd", "tcp")) == NULL) {
        fprintf(stderr, "No se pudo obtener el puerto para el servicio qotd\n");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = qotd_servent->s_port;
    if (inet_aton(argv[1], &addr.sin_addr) == 0) {
        fprintf(stderr, "Dirección IP invalida: %s\n", argv[1]);
        exit(1);
    }

    //TODO: Mirar a ver si se puede hacer de otra forma
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = qotd_servent->s_port;
    myaddr.sin_addr.s_addr = INADDR_ANY;

    //TODO: Meterlo dentro de un switch para permitir escalabilidad a TCP a futuro.
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Error al crear el socket");
        exit(1);
    }

    //TODO: Obtener IP y puerto desde getaddrinfo() en lugar de armar la estructura a mano.
    if (bind(sockfd, (struct sockaddr *) &myaddr, (socklen_t) sizeof(myaddr)) == -1) {
        fprintf(stderr, "Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    //TODO: Hacer una constante con el mensaje a enviar (el servidor ignora el cuerpo del mensaje)
    if (sendto(sockfd, "Enviame el mensaje del dia\0", 27, 0, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) == -1) {
        fprintf(stderr, "Error al enviar el mensaje");
        printf("Errno: %d\n", errno);
        close(sockfd);
        exit(1);
    }

    if (recvfrom(sockfd, msg, 0, 0, NULL, NULL) == -1) {
        fprintf(stderr, "Error al recibir el mensaje");
        close(sockfd);
        exit(1);
    }

    printf("Mensaje del dia: %s\n", msg);
    close(sockfd);
    return 0;

//TODO: crear dos etiquetas, la primera para error con close(sockfd), sin break,
// la segunda para exit(1)
}

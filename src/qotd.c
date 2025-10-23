#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>
#include <netdb.h>

#include "argparse.h"
#include "config.h"
#include "qotd.h"


int
qotd_setup_socket(
    struct arguments   *args,
    struct sockaddr_in *myaddr,
    struct sockaddr_in *addr
){
    struct servent *qotd_servent;
    int sockfd;

    /* Use tcp because some linux distributions only offer qotd over tcp 
     * listed in /etc/services, udp port does not appear
     */
    if ((qotd_servent = getservbyname(args->service, "tcp")) == NULL) {
        fprintf(stderr, "Could not resolve QOTD's port number\n");
        exit(1);
    }

    addr->sin_family = AF_INET;
    addr->sin_port = qotd_servent->s_port;
    if (inet_aton(args->ip_address, &addr->sin_addr) == 0) {
        fprintf(stderr, "Invalid IP address: %s \n", args->ip_address);
        exit(1);
    }

    myaddr->sin_family = AF_INET;
    myaddr->sin_port = qotd_servent->s_port;
    myaddr->sin_addr.s_addr = INADDR_ANY;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *) myaddr, (socklen_t) sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
    }

    return sockfd;
}


int
qotd_get_quote
(
    int sockfd,
    struct sockaddr_in *addr,
    char **received_msg
){
    int err;
    ssize_t msg_size;

    // Send request message
    err = sendto(sockfd, STRING_QOTD_MSG, strlen(STRING_QOTD_MSG), 0,
            (struct sockaddr *)addr, (socklen_t)sizeof(*addr));
    if (err == -1) {
        perror("sendto");
        goto exit_error_socket;
    }

    /* Peek the message to find out the actual size of it.
     * From the manual pages, MSG_TRUNC flag makes recvfrom return the real size
     * without removing the datagram from the queue. This is useful because
     * recvfrom() doesn't do reallocs, so you would need to allocate initially
     * a buffer of 64KB to account for the maximum size of an UDP packet
     */
    msg_size = recvfrom(sockfd, NULL, 0, MSG_PEEK | MSG_TRUNC, NULL, NULL);
    if (msg_size == -1) {
        perror("recvfrom peek");
        goto exit_error_socket;
    }

    // Allocate memory for the message buffer
    *received_msg = (char *)malloc(msg_size);
    if (!received_msg) {
        perror("malloc");
        goto exit_error_socket;
    }

    // Actually fetch the packet from the queue onto the allocated buffer.
    msg_size = recvfrom(sockfd, *received_msg, msg_size, 0, NULL, NULL);
    if (err == -1) {
        perror("recvfrom");
        goto exit_error_socket;
    }

    return 0;

exit_error_socket:
    if (received_msg)
        free(received_msg);
    close(sockfd);
    exit(1);
}
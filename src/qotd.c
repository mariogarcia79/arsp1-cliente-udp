#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>, not needed
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

    /*
     * Fill servent structure to get QOTD port number.
     * The port number is obtained from /etc/services file using getservbyname()
     */
    if ((qotd_servent = getservbyname(args->service, "udp")) == NULL) {
        fprintf(stderr, "Could not resolve QOTD's port number\n");
        return -1;
    }

    /*
     * Fill sockaddr_in structure with the server address.
     * The IP address is obtained from command line arguments and formatted
     * using inet_aton().
     */
    addr->sin_family = AF_INET;
    addr->sin_port = qotd_servent->s_port;
    if (inet_aton(args->ip_address, &addr->sin_addr) == 0) {
        fprintf(stderr, "Invalid IP address: %s \n", args->ip_address);
        return -1;
    }

    /*
     * Fill sockaddr_in structure with the client address (used for binding).
     * INADDR_ANY is used to bind to all interfaces.
     */
    myaddr->sin_family = AF_INET;
    myaddr->sin_port = qotd_servent->s_port;
    myaddr->sin_addr.s_addr = INADDR_ANY;

    // Open UDP socket with SOCK_DGRAM flag
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    // Bind the client address to the socket
    if (bind(sockfd, (struct sockaddr *) myaddr, (socklen_t) sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
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

    /* 
     * Peek the message to find out the actual size of it.
     * From the manual pages, MSG_PEEK reads the datagram without removing it
     * from the message queue. MSG_TRUNC flag makes recvfrom return the real size
     * of that datagram.
     * This is useful because recvfrom() doesn't do reallocs, so you would need 
     * to allocate initially a buffer of 64KB to account for the maximum size of 
     * an UDP packet. This is quite wasteful in terms of memory usage.
     */
    msg_size = recvfrom(sockfd, NULL, 0, MSG_PEEK | MSG_TRUNC, NULL, NULL);
    if (msg_size == -1) {
        perror("recvfrom peek");
        goto exit_error_socket;
    }

    // Allocate memory for the message buffer using the previously obtained size
    *received_msg = (char *)malloc(msg_size);
    if (!received_msg) {
        perror("malloc");
        goto exit_error_socket;
    }

    /*
     * Actually fetch the packet from the queue onto the allocated buffer.
     * We don't need to use the address of the sender, so we pass NULL as those
     * arguments.
     */
    msg_size = recvfrom(sockfd, *received_msg, msg_size, 0, NULL, NULL);
    if (err == -1) {
        perror("recvfrom");
        goto exit_error_socket;
    }

    return 0;

// clean exit
exit_error_socket:
    if (received_msg)
        free(received_msg);
    close(sockfd);
    return -1;
}
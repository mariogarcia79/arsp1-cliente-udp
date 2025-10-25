#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>, not needed
#include <netdb.h>

#ifndef QOTD_H
#define QOTD_H

#define STRING_QOTD_MSG "Enviame el mensaje del dia"
// Sizeof is resolved at compile time, effectively serving as strlen-like macro
#define STRING_QOTD_MSG_LEN \
    ( (sizeof(STRING_QOTD_MSG)/sizeof(STRING_QOTD_MSG[0])) - sizeof(STRING_QOTD_MSG[0]) )


int qotd_setup_socket
(
    struct arguments   *args,
    struct sockaddr_in *myaddr,
    struct sockaddr_in *addr
);

int qotd_get_quote
(
    int sockfd,
    struct sockaddr_in *addr,
    char **received_msg
);

#endif /* QOTD_H */

#ifndef ARGPARSE_H
#define ARGPARSE_H

#define STRING_USAGE "Usage: %s <IP address> [-s service]\n"

struct arguments {
    char *program_name;
    char *ip_address;
    char *service;
};


void print_usage
(
    const char *program_name
);

void print_help(void);

int parse_args
(
    int argc,
    char *argv[],
    struct arguments *args
);

#endif /* ARGPARSE_H */

#ifndef CONFIG_H
#define CONFIG_H

#define MAX_IPV4_LENGTH 15
#define MAX_SERVICE_LENGTH 12

#define SERVICE_DEFAULT "qotd"

#endif /* CONFIG_H */

void print_help(void);
void init_defaults(char *prog_name, struct arguments *args);

typedef enum {
    FLAG_UNKNOWN = -1,
    FLAG_HELP    = 0,
    FLAG_SERVICE = 1
} FlagType;

FlagType
get_flag(const char *arg)
{
    if (!strcmp(arg, "-h"))        return FLAG_HELP;
    if (!strcmp(arg, "--help"))    return FLAG_HELP;
    if (!strcmp(arg, "-s"))        return FLAG_SERVICE;
    if (!strcmp(arg, "--service")) return FLAG_SERVICE;
    return FLAG_UNKNOWN;
}

void
init_defaults(char *prog_name, struct arguments *args)
{
    args->program_name = prog_name;
    args->service = SERVICE_DEFAULT;
    args->ip_address = NULL;
}

/* 
 * parse_args()
 * Parses command line arguments and fills the arguments struct.
 * Returns 0 on success, -1 on failure setting errno.
 */
int
parse_args(int argc, char *argv[], struct arguments *args)
{
    int count = 1;
    
    init_defaults(argv[0], args);

    // Precondition
    if (argc < 2)
        goto exit_error_invalid;
    
    while (count < argc) {
        if (argv[count][0] == '-') {
            // It's a flag, handle accordingly
            switch (get_flag(argv[count])) {
                case FLAG_HELP:
                    print_usage(args->program_name);
                    print_help();
                    goto exit_success;
                case FLAG_SERVICE:
                    count++;
                    if (count >= argc)
                        goto exit_error_invalid;
                    else
                        args->service = argv[count];
                    break;
                default:
                    goto exit_error_invalid;
            }
        } else {
            // It's not a flag, it must be the IP address (set only once)
            if (!args->ip_address)
                args->ip_address = argv[count];
            else
                goto exit_error_invalid;            
        }
        count++;
    }

    if (args->ip_address == NULL) {
        goto exit_error_invalid;
    }

exit_success:
    return 0;
exit_error_invalid:
    errno = EINVAL;
    return -1;
}

void
print_usage(const char *program_name)
{
    fprintf(stdout, STRING_USAGE, program_name);
}

void
print_help(void)
{
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -h, --help               Show this help message and exit\n");
    fprintf(stdout, "  -s, --service <service>  Specify the service to use (default: %s)\n", SERVICE_DEFAULT);
}

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

typedef enum {
    SERVICE_UNKNOWN,
    SERVICE_QOTD
} ServiceType;

ServiceType
get_service_type(const char *name)
{
    if (strcmp(name, "qotd") == 0) return SERVICE_QOTD;
    return SERVICE_UNKNOWN;
}

int
main (int argc, char *argv[])
{
    struct sockaddr_in myaddr = {0};
    struct sockaddr_in addr   = {0};
    struct arguments   args   = {0};
    char *received_msg = NULL;
    ServiceType service;
    int sockfd;
    int err;
    
    err = parse_args(argc, argv, &args);
    if (err == -1) {
        perror("parse_args");
        print_usage(argv[0]);
        exit(1);
    }
    service = get_service_type(args.service);
    
    switch(service) {
        case SERVICE_QOTD:
            sockfd = qotd_setup_socket(&args, &myaddr, &addr);
            if (sockfd == -1) exit(1);
            err = qotd_get_quote(sockfd, &addr, &received_msg);
            if (err == -1) exit(1);
            printf("%s\n", received_msg);
            if (received_msg)
                free(received_msg);
            close(sockfd);
            break;
	case SERVICE_UNKNOWN:
        default:
            fprintf(stderr, "Service not found\n");
            exit(1);
    }
    
    return 0;
}

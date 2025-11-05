#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>, not needed
#include <netdb.h>

#define MAX_IPV4_LENGTH    15
#define MAX_SERVICE_LENGTH 12
#define MAX_BUFFER_SIZE    512

#define STRING_USAGE "Usage: %s <IP address> [-s service]\n"
#define STRING_QOTD_MSG "Enviame el mensaje del dia"
#define CHAR_SIZE sizeof(STRING_QOTD_MSG[0])
// Sizeof is resolved at compile time, effectively serving as strlen-like macro
#define STRING_QOTD_MSG_LEN \
    ( (sizeof(STRING_QOTD_MSG)/CHAR_SIZE) - CHAR_SIZE )

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
    fprintf(stdout, "  -s, --service <service>  Specify the service to use (default: qotd)\n");
}

struct arguments {
    char *program_name;
    char *ip_address;
    char *service;
};

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
    args->service = "qotd";
    args->ip_address = NULL;
}

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
    if ((qotd_servent = getservbyname(args->service, "tcp")) == NULL) {
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

    // Open TCP socket with SOCK_STREAM flag
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    
    // Bind the client address to the socket
    if (bind(sockfd, (struct sockaddr *) myaddr, (socklen_t) sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) addr, (socklen_t) sizeof(*addr)) == -1) {
        perror("connect");
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
    char *received_msg
){
    int err;
    ssize_t msg_size;


    err = send(sockfd, STRING_QOTD_MSG, strlen(STRING_QOTD_MSG), 0);
    if (err == -1) {
        perror("send");
        goto exit_error_socket;
    }

    /*
     * Actually fetch the packet from the queue onto the fixed-length buffer.
     * As TCP is connection-oriented, we don't use MSG_TRUNC as we did with UDP
     * because we actually can receive bytes continously, and we don't have the
     * concept of datagram. Therefore, it doesn't make sense to get the actual
     * size we are receiving, so we assume a maximum packet size of 512 bytes.
     * This size is stated in MAX_BUFFER_SIZE.
     * We don't need to use the address of the sender, so we pass NULL as those
     * arguments.
     */
    msg_size = recv(sockfd, received_msg, MAX_BUFFER_SIZE, 0);
    if (msg_size == -1) {
        perror("recv");
        goto exit_error_socket;
    }

    if (shutdown(sockfd, SHUT_RDWR) == -1) {
        perror("shutdown");
        goto exit_error_socket;
    }

    close(sockfd);
    return 0;

// clean exit
exit_error_socket:
    // No more receptions or transmissions
    if (shutdown(sockfd, SHUT_RDWR) == -1)
        perror("shutdown");
    close(sockfd);
    return -1;
}

int
main (int argc, char *argv[])
{
    struct sockaddr_in myaddr = {0};
    struct sockaddr_in addr   = {0};
    struct arguments   args   = {0};
    char received_msg[MAX_BUFFER_SIZE] = {0};
    int sockfd;
    int err;

    if (parse_args(argc, argv, &args) == -1) {
        perror("parse_args");
        print_usage(argv[0]);
        exit(1);
    }

    if (strcmp(args.service, "qotd")) {
        fprintf(stderr, "Service not found\n");
        exit(1);
    }

    sockfd = qotd_setup_socket(&args, &myaddr, &addr);
    if (sockfd == -1) exit(1);

    err = qotd_get_quote(sockfd, &addr, received_msg);
    if (err == -1) exit(1);
    
    printf("%s\n", received_msg);

    return 0;
}

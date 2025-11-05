#ifndef QOTD_H
#define QOTD_H

#define STRING_QOTD_MSG "Send me the message of the day."
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

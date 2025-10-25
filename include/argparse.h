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
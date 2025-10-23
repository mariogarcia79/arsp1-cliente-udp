#ifndef ARGPARSE_H
#define ARGPARSE_H

#define STRING_USAGE "Usage: %s <IP address> [-s service]\n"

struct arguments {
    char *program_name;
    char *ip_address;
    char *service;
};

int get_flag(char *arg);

int run_checks
(
    int argc,
    char *argv[],
    struct arguments *args
);

int parse_args
(
    int argc,
    char *argv[],
    struct arguments *args
);


#endif /* ARGPARSE_H */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "argparse.h"
#include "config.h"


int
parse_args(int argc, char *argv[], struct arguments *args)
{
    if (run_checks(argc, argv, args) == -1) {
        perror("argparser");
        fprintf(stderr, STRING_USAGE, argv[0]);
        exit(1);
    }

    args->program_name = argv[0];
    args->ip_address   = argv[1];

    return 0;
}

int
run_checks
(
    int argc,
    char *argv[],
    struct arguments *args
){
    // Validate argument count
    switch (argc) {
        case 2:
            args->service = SERVICE_DEFAULT;
            break;
        case 5:
            if (!get_flag(argv[3])) {
                // Validate service name length
                if (strlen(argv[4]) > MAX_SERVICE_LENGTH)
                    goto exit_error_einval;
                else 
                    args->service = argv[4];
                break;
            } else {
                goto exit_error_einval;
            }
        default:
            goto exit_error_einval;        
    }

    // Validate IP address length
    if (strlen(argv[1]) > MAX_IPV4_LENGTH) {
        goto exit_error_einval;
    }

    return 0;

exit_error_einval:
    errno = EINVAL;
    return -1;
}


int
get_flag(char *arg)
{
    if (!strcmp(arg, "-s"))
        return 0;
    else
        return -1;
}
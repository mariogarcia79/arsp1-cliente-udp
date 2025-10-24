#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "argparse.h"
#include "config.h"

/* parse_args
 * Parses command line arguments and fills the arguments struct.
 * Returns 0 on success, -1 on failure setting errno.
 */
int
parse_args(int argc, char *argv[], struct arguments *args)
{
    if (run_checks(argc, argv, args) == -1) {
        return -1;
    }

    args->program_name = argv[0];
    args->ip_address   = argv[1];

    return 0;
}

int
run_checks(int argc, char *argv[], struct arguments *args)
{
    // Validate argument count
    switch (argc) {
        case 2:
            args->service = SERVICE_DEFAULT;
            break;
        case 4:
            // Check for -s flag and validate service length
            if (!get_flag(argv[2]) &&
            strlen(argv[3]) < MAX_SERVICE_LENGTH) {
                printf("hasta aqui bien\n");
                args->service = argv[4];
                break;
            }
            // fallthrough
        default:
            errno = EINVAL;
            return -1;
    }

    // Validate IP address length
    if (strlen(argv[1]) > MAX_IPV4_LENGTH) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int
get_flag(char *arg)
{
    return !strcmp(arg, "-s") ? 0 : -1;
}
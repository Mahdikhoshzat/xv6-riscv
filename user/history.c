#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    int arg = atoi(argv[1]);
    history(arg);
    exit(0);
}
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc <= 1) {
        fprintf(2, "Command not specified. Abort...\n");
    }

    int sleep_time_seconds = 2;
    int command_start = 1;
    if(argc > 3 && strcmp(argv[1], "-n") == 0) {
        sleep_time_seconds = atoi(argv[2]);
        command_start = 3;
    }

    for(;;) {
        int pid = fork();
        if(pid == 0) {
            exec(argv[command_start], &argv[command_start]);
            fprintf(2, "Command not found. Abort...\n");
            exit(1);
        }
        wait(0);
        pause(sleep_time_seconds * 10);
    }

    exit(0);
}
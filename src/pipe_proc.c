#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include "pipe_proc.h"

static void exec_child(const char* cmd, char* const* argv,
                       int inpipefd[2], int outpipefd[2]) {
    close(outpipefd[1]);
    close(inpipefd[0]);
    dup2(outpipefd[0], STDIN_FILENO);
    dup2(inpipefd[1], STDOUT_FILENO);
    close(outpipefd[0]);
    close(inpipefd[1]);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    execv(cmd, argv);
}

int pipe_proc(const char* cmd, char* const* argv, FILE** in, FILE** out) {
    int pid;
    int outpipefd[2] = {-1, -1};
    int inpipefd[2] = {-1, -1};

    if (pipe(outpipefd) || pipe(inpipefd)) {
        fprintf(stderr, "Error: can't create pipes\n");
        return 0;
    } else if ((pid = fork()) < 0) {
        fprintf(stderr, "Error: fork failed\n");
        return 0;
    } else if (!pid) {
        exec_child(cmd, argv, inpipefd, outpipefd);
        fprintf(stderr, "Error: failed to execute command: %s\n", cmd);
        return -1;
    } else {
        close(outpipefd[0]);
        close(inpipefd[1]);

        if (!(*out = fdopen(inpipefd[0], "r")) || !(*in = fdopen(outpipefd[1], "w"))) {
            if (*in) fclose(*in);
            if (*out) fclose(*out);
            return 0;
        }
    }
    return pid;
}

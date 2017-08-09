/*
 * Copyright (C) 2017 Andrew Jeffery
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const char get_offset_fmt[] =
"#include <stddef.h>\n"
"#include <stdio.h>\n"
"#include <unistd.h>\n"
"int main(int argc, char *argv[]) {\n"
"       printf(\"%%s.%%s: %%lu\\n\", \"%s\", \"%s\", offsetof(struct %s, %s));\n"
"       unlink(argv[0]);\n"
"       return 0;\n"
"}\n";

const char *usage =
"Usage: %s <STRUCT NAME> <MEMBER NAME>\n"
"\n"
"  Struct definition required on stdin\n"
"\n"
"Example:\n"
"\n"
"  $ %s a d <<< \"struct a {int b; char c; int d; };\"\n"
"  a.d: 8\n"
"\n";

char stemp[] = "csmo.XXXXXX.c";

int main(int argc, char *argv[], char *envp[]) {
    char *get_offset;
    char *args[5];
    int wstatus;
    int rv = 0;
    char *line;
    pid_t pid;
    char *bin;
    int fd;

    if (argc < 3) {
        printf(usage, argv[0], argv[0]);
        return 1;
    }

    /* FIXME: Pick a better (?!) line length. Or don't do line-based stuff */
    line = calloc(1, 1024);
    if (!line) {
        perror("calloc");
        return 1;
    }

    fd = mkstemps(stemp, 2);
    if (fd == -1) {
        perror("mkstemp");
        return 1;
    }

    while (line = fgets(line, 1024, stdin)) {
        rv = write(fd, line, strlen(line));
        if (rv == -1) {
            perror("write");
            goto cleanup_mkstemp;
        }
    }

    rv = asprintf(&get_offset, get_offset_fmt, argv[1], argv[2], argv[1], argv[2]);
    if (rv == -1) {
        perror("asprintf");
        goto cleanup_mkstemp;
    }

    rv = write(fd, get_offset, strlen(get_offset));

    free(get_offset);

    rv = asprintf(&bin, "%s.bin", stemp);
    if (rv == -1) {
        perror("asprintf");
        goto cleanup_mkstemp;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        rv = 1;
        goto cleanup_bin;
    } else if (pid == 0) {
        args[0] = "/usr/bin/cc";
        args[1] = stemp;
        args[2] = "-o";
        args[3] = bin;
        args[4] = NULL;

        rv = execve("/usr/bin/cc", args, envp);
        if (rv == -1) {
            perror("execve");
            goto cleanup_bin;
        }
    }

    waitpid(pid, &wstatus, 0);
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0) {
        printf("Child exited %d\n", WEXITSTATUS(wstatus));
        rv = 1;
        goto cleanup_bin;
    }

    close(fd);
    unlink(stemp);

    args[0] = bin;
    args[1] = NULL;

    rv = execve(bin, args, envp);
    if (rv == -1)
        perror("execve");

cleanup_bin:
    free(bin);

cleanup_mkstemp:
    close(fd);
    unlink(stemp);

    return !!rv;
}

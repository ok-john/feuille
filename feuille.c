/*
 * feuille.c
 *  Main source file.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#define _DEFAULT_SOURCE

#include "feuille.h"

#ifndef COSMOPOLITAN
#include <errno.h>     /* for errno, ERANGE, EAGAIN, EFBIG, ENOENT              */
#include <grp.h>       /* for initgroups                                        */
#include <limits.h>    /* for USHRT_MAX, ULONG_MAX, CHAR_MAX, PATH_MAX, UCHA... */
#include <locale.h>    /* for NULL, setlocale, LC_ALL                           */
#include <pwd.h>       /* for getpwnam, passwd                                  */
#include <signal.h>    /* for signal, SIGPIPE, SIG_IGN                          */
#include <stdio.h>     /* for puts                                              */
#include <stdlib.h>    /* for strtoll, free, realpath, srand                    */
#include <string.h>    /* for strerror, strlen                                  */
#include <sys/stat.h>  /* for mkdir                                             */
#include <sys/wait.h>  /* for wait                                              */
#include <syslog.h>    /* for syslog, openlog, LOG_WARNING, LOG_NDELAY, LOG_... */
#include <time.h>      /* for time                                              */
#include <unistd.h>    /* for getuid, access, chdir, chown, chroot, close       */
#endif

#include "arg.h"       /* for EARGF, ARGBEGIN, ARGEND                           */
#include "bin.h"       /* for create_url, generate_id, write_paste              */
#include "server.h"    /* for send_response, accept_connection, close_connec... */
#include "util.h"      /* for verbose, die, error                               */

char    *argv0;

/* default settings */
Settings settings = {
    .address             = "0.0.0.0",
    .url                 = "http://localhost",
    .output              = "/var/www/feuille",
    .user                = "www",

    .id_length           = 4,
    .worker_count        = 4,
    .port                = 9999,
    .timeout             = 2,
    .max_size            = 1048576, /* = 1MiB   = 1024 * 1024 */
    .buffer_size         = 131072,  /* = 128KiB = 1024 * 128  */

    .verbose             = 0,
    .foreground          = 0
};

/* functions declarations */
static  void     usage(int exit_code);
static  void     version(void);
static  void     accept_loop(int);

/**
 * Display feuille's basic usage.
 *   exit_code: the exit code to be used.
 */
void usage(int exit_code)
{
    die(exit_code, "usage: %s [-abfhiopstuUvVw]\n"
                   "       see `man feuille'.\n", argv0);
}

/**
 * Display feuille's author(s) and version.
 */
void version(void)
{
    die(0, "%s %s by Tom MTT. <tom@heimdall.pm>\n", argv0, VERSION);
}

/**
 * Feuille's accept loop.
 *   server: the server socket.
 */
void accept_loop(int server)
{
    /* get current process' pid */
    int pid = getpid();

    /* feed the random number god */
    srand(time(0) + pid);

    /* accept loop */
    int connection;
    while ((connection = accept_connection(server))) {
        /* check if the socket is invalid */
        if (connection == -1) {
            error("error while accepting incoming connection: %s", strerror(errno));
            continue;
        }

        verbose(1, "--- new incoming connection. connection ID: %d:%d ---", pid, time(0));

        unsigned long paste_size = 0;

        char *paste = NULL;
        char *id    = NULL;
        char *url   = NULL;

        /* read paste from connection */
        verbose(1, "reading paste from incoming connection...");

        if ((paste_size = read_paste(connection, &paste)) != 0) {
            /* generate random ID */
            verbose(1, "done.");
            verbose(2, "generating a random ID...");

            if ((id = generate_id(settings.id_length)) != NULL) {
                /* write paste to disk */
                verbose(2, "done.");
                verbose(1, "writing paste `%s' to disk...", id);

                if (write_paste(paste, paste_size, id) == 0) {
                    /* create URL */
                    verbose(1, "done.");
                    verbose(2, "making the right URL...");

                    if ((url = create_url(id)) != NULL) {
                        /* send URL */
                        verbose(2, "done.", url);
                        verbose(1, "sending the link to the client...");

                        send_response(connection, url);

                        verbose(1, "All done.");

                        free(url);
                    } else {
                        error("error while making a valid URL.");
                        send_response(connection, "Could not create your paste URL.\nPlease try again later.\n");
                    }
                } else {
                    error("error while writing paste to disk.");
                    send_response(connection, "Could not write your paste to disk.\nPlease try again later.\n");
                }

                free(id);
            } else {
                error("error while generating a random ID.");
                send_response(connection, "Could not generate your paste ID.\nPlease try again later.\n");
            }

            free(paste);
        } else {
            if (errno == EFBIG)
                send_response(connection, "Paste too big.\n");

            if (errno == ENOENT)
                send_response(connection, "Empty paste.\n");

            if (errno == EAGAIN)
                send_response(connection, "Timeout'd.\n");

            error("error %d while reading paste from incoming connection.", errno);
        }

        /* close connection */
        close_connection(connection);
    }
}

/**
 * Feuille's main function.
 *   argc: the argument count.
 *   argv: the argument values.
 * -> an exit code.
 */
int main(int argc, char *argv[])
{
    /* locale */
    setlocale(LC_ALL, "");

    /* syslog */
    openlog("feuille", LOG_NDELAY | LOG_PERROR, LOG_USER);

    /* ignore signals that could kill feuille */
    signal(SIGPIPE, SIG_IGN); /* when send(2) or write(2) fails */


    /* settings */
    long long tmp;

    /* set number of workers */
    if ((tmp = sysconf(_SC_NPROCESSORS_ONLN)) > settings.worker_count && tmp <= USHRT_MAX)
        settings.worker_count = tmp;

    ARGBEGIN {
    case 'a':
        /* set address */
        settings.address = EARGF(usage(1));
        break;

    case 'b':
        /* set buffer size */
        tmp = strtoll(EARGF(usage(1)), NULL, 10);

        if (tmp <= 0 || tmp > ULONG_MAX || errno == ERANGE)
            die(ERANGE, "invalid buffer size.\n"
                        "see `man feuille'.\n");

        settings.buffer_size = tmp;
        break;

    case 'f':
        /* enable foreground execution */
        settings.foreground = 1;
        break;

    case 'h':
        /* get help */
        usage(0);
        break;

    case 'i':
        /* set id length */
        tmp = strtoll(EARGF(usage(1)), NULL, 10) + 1;

        if (tmp - 1 < 4 || tmp > UCHAR_MAX || errno == ERANGE)
            die(ERANGE, "invalid id length.\n"
                        "see `man feuille'.\n");

        settings.id_length = tmp;
        break;

    case 'o':
        /* set output folder */
        settings.output = EARGF(usage(1));
        break;

    case 'p':
        /* set port */
        tmp = strtoll(EARGF(usage(1)), NULL, 10);

        if (tmp <= 0 || tmp > USHRT_MAX || errno == ERANGE)
            die(ERANGE, "invalid port.\n"
                        "see `man feuille'.\n");

        settings.port = tmp;
        break;

    case 's':
        /* set max size */
        tmp = strtoll(EARGF(usage(1)), NULL, 10);

        if (tmp <= 0 || tmp > ULONG_MAX || errno == ERANGE)
            die(ERANGE, "invalid maximum size.\n"
                        "see `man feuille'.\n");

        settings.max_size = tmp;
        break;

    case 't':
        /* set timeout */
        tmp = strtoll(EARGF(usage(1)), NULL, 10);

        if (tmp < 0 || tmp > UINT_MAX || errno == ERANGE)
            die(ERANGE, "invalid timeout.\n"
                        "see `man feuille'.\n");

        settings.timeout = tmp;
        break;

    case 'u':
        /* set user */
        settings.user = EARGF(usage(1));
        break;

    case 'U':
        /* set url */
        settings.url = EARGF(usage(1));

        if (settings.url[strlen(settings.url) - 1] == '/')
            settings.url[strlen(settings.url) - 1] = 0;

        break;

    case 'v':
        /* enable verbose mode */
        if (settings.verbose == CHAR_MAX)
            die(ERANGE, "why? just why?\n"
                        "please see `man feuille' and go touch grass.\n");

        settings.verbose++;
        break;

    case 'V':
        /* get version */
        version();
        break;

    case 'w':
        /* set worker count */
        tmp = strtoll(EARGF(usage(1)), NULL, 10);

        if (tmp <= 0 || tmp > USHRT_MAX || errno == ERANGE)
            die(ERANGE, "invalid worker count.\n"
                        "see `man feuille'.\n");

        settings.worker_count = tmp;
        break;

    default:
        usage(1);
    } ARGEND;

    if (argc != 0)
        usage(1);


    /* output folder checks */
    char path[PATH_MAX];

    if (mkdir(settings.output, 0755) == 0)
        verbose(2, "creating folder `%s'...", settings.output);

    if (realpath(settings.output, path) == NULL)
        die(errno, "could not get real path of directory `%s': %s.\n", settings.output, strerror(errno));

    if (access(path, W_OK) != 0)
        die(errno, "cannot write to directory `%s': %s.\n", path, strerror(errno));

    chdir(path);

    /* user checks */
    uid_t uid = 0;
    gid_t gid = 0;
    if (getuid() == 0) {
        if (strlen(settings.user) == 0)
            settings.user = "nobody";

        verbose(2, "getting uid and gid of user `%s'...", settings.user);

        struct passwd *user;
        if ((user = getpwnam(settings.user)) == NULL)
            die(1, "user `%s' doesn't exist.\n", settings.user);

        uid = user->pw_uid;
        gid = user->pw_gid;

    } else {
        syslog(LOG_WARNING, "running as non-root user.");
        syslog(LOG_WARNING, "`chroot' and user switching have been disabled.");
        puts("");
    }


    /* server socket creation (before dropping root permissions) */
    verbose(1, "initializing server socket...");

    int server;
    if ((server = initialize_server()) == -1)
        die(errno, "failed to initialize server socket: %f.\n", strerror(errno));


    /* make feuille run in the background */
    if (!settings.foreground) {
        verbose(1, "making feuille run in the background...");
        verbose(2, "closing input / output file descriptors...");

        daemon(1, 0);
    }


    /* chroot and drop root permissions */
    if (getuid() == 0) {
        verbose(2, "setting owner of `%s' to `%s'...", path, settings.user);
        chown(path, uid, gid);

        /* chroot */
        verbose(2, "chroot'ing into `%s'...", path);
        chroot(path);

        /* privileges drop */
        verbose(2, "dropping root privileges...");

        /* switching groups */
        if (setgid(gid) != 0 || getgid() != gid)
            die(1, "could not switch to group for user `%s'.\n", settings.user);

        #ifndef COSMOPOLITAN
        /* initgroups doesn't work on cosmopolitan libc yet */
        if (initgroups(settings.user, gid) != 0)
            die(1, "could not initialize other groups for user `%s'.\n", settings.user);
        #endif

        /* switching user */
        if (setuid(uid) != 0 || getuid() != uid)
            die(1, "could not switch to user `%s'.\n", settings.user);
    }

#if defined __OpenBSD__ || defined COSMOPOLITAN
    /* OpenBSD-only security measures */
    pledge("proc stdio rpath wpath cpath inet", "stdio rpath wpath cpath inet");
#endif


#ifdef DEBUG
    /* do not create a thread pool if in DEBUG mode */
    verbose(1, "running in DEBUG mode, won't create a worker pool.");
    accept_loop(server);
#else
    /* create a thread pool for incoming connections */
    verbose(1, "initializing worker pool...");

    int pid;
    for (int i = 1; i <= settings.worker_count; i++) {
        if ((pid = fork()) == 0) {
            verbose(2, "  worker n. %d...", i);
            accept_loop(server);

        } else if (pid < 0)
            die(errno, "could not initialize worker n. %d: %s.\n", i, strerror(errno));
    }

    sleep(1);

    verbose(1, "all workers have been initialized.");
    verbose(1, "beginning to accept incoming connections.");


    /* fork again if a child dies */
    int status;
    int child_pid;
    while ((child_pid = wait(&status)) > 0) {
        error("child %d unexpectedly died with exit code %d.", child_pid, WEXITSTATUS(status));

        /* do not fork if child was KILL'ed */
        if (WTERMSIG(status) == 9)
            continue;

        if ((pid = fork()) == 0) {
            accept_loop(server);

        } else if (pid < 0)
            error("could not fork killed child again: ", strerror(errno));
    }
#endif

    close(server);
    return 0;
}

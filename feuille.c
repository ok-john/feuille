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

#include <errno.h>     /* for errno, ERANGE, EFBIG, ENOENT                      */
#include <limits.h>    /* for USHRT_MAX, ULONG_MAX, CHAR_MAX, PATH_MAX, UCHA... */
#include <locale.h>    /* for NULL, setlocale, LC_ALL                           */
#include <pwd.h>       /* for getpwnam, passwd                                  */
#include <stdio.h>     /* for freopen, puts, stderr, stdin, stdout              */
#include <stdlib.h>    /* for strtoll, exit, free, realpath, srand              */
#include <string.h>    /* for strerror, strlen                                  */
#include <sys/stat.h>  /* for mkdir                                             */
#include <sys/wait.h>  /* for wait                                              */
#include <syslog.h>    /* for syslog, openlog, LOG_WARNING, LOG_NDELAY, LOG_... */
#include <time.h>      /* for time                                              */
#include <unistd.h>    /* for fork, access, chdir, chown, chroot, close, getpid */

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
    .worker_count        = 1,
    .port                = 9999,
    .timeout             = 4,
    .max_size            = 2097152, /* = 2MiB   = 1024 * 1024 * 2 */
    .buffer_size         = 131072,  /* = 128KiB = 1024 * 128      */

    .verbose             = 0,
    .foreground          = 0
};

/* functions declarations */
static  void     usage(int exit_code);
static  void     version(void);

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
 * Feuille's main function.
 *   argc: the argument count.
 *   argv: the argument values.
 * -> an exit code.
 */
int main(int argc, char *argv[])
{
    /* locale */
    setlocale(LC_ALL, "");

    /* syslog setup */
    openlog("feuille", LOG_NDELAY |  LOG_PERROR, LOG_USER);

    /* settings */
    long long tmp;

    /* set number of workers */
    if ((tmp = sysconf(_SC_NPROCESSORS_ONLN)) > 0 && tmp <= USHRT_MAX)
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
        die(errno, "Could not get real path of directory `%s': %s\n", settings.output, strerror(errno));

    if (access(path, W_OK) != 0)
        die(errno, "Cannot write to directory `%s': %s\n", path, strerror(errno));

    chdir(path);

    /* user checks */
    int uid, gid;
    if (getuid() == 0) {
        if (strlen(settings.user) == 0)
            settings.user = "nobody";

        verbose(2, "getting uid and gid of user `%s'...", settings.user);

        struct passwd *user;
        if ((user = getpwnam(settings.user)) == NULL)
            die(1, "User `%s' doesn't exist\n", settings.user);

        uid = user->pw_uid;
        gid = user->pw_gid;
    } else {
        puts("");
        syslog(LOG_WARNING, "running as non-root user.");
        syslog(LOG_WARNING, "`chroot' and user switching have been disabled.");
        puts("");
    }


    /* server socket creation (before dropping root permissions) */
    verbose(1, "initializing server socket...");

    int server;
    if ((server = initialize_server()) == -1)
        die(errno, "Failed to initialize server socket: %s\n", strerror(errno));

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
        setgid(gid);
        setuid(uid);

    }

    /* OpenBSD-only security measures */
    #ifdef __OpenBSD__
    pledge("proc stdio rpath wpath cpath inet", "stdio rpath wpath cpath inet");
    #endif


    /* create a thread pool for incoming connections */
    verbose(1, "initializing worker pool...");

    int pid;
    for (int i = 1; i <= settings.worker_count; i++) {
        if ((pid = fork()) == 0) {
            verbose(2, "  worker n. %d...", i);

            pid = getpid();

            /* feed the random number god */
            srand(time(0) + pid);

            /* accept loop */
            int connection;
            while ((connection = accept_connection(server))) {
                verbose(1, "--- new incoming connection. connection ID: %d:%d ---", pid, time(0));

                char *paste = NULL;
                char *id    = NULL;
                char *url   = NULL;

                /* read paste from connection */
                verbose(1, "reading paste from incoming connection...");

                if ((paste = read_paste(connection)) != NULL) {
                    /* generate random ID */
                    verbose(1, "done.");
                    verbose(2, "generating a random ID...");

                    if ((id = generate_id(settings.id_length)) != NULL) {
                        /* write paste to disk */
                        verbose(2, "done.");
                        verbose(1, "writing paste `%s' to disk...", id);

                        if (write_paste(paste, id) == 0) {
                            /* create URL */
                            verbose(1, "done.");
                            verbose(2, "making the right URL...");

                            if ((url = create_url(id)) != NULL) {
                                verbose(2, "done.", url);
                                verbose(1, "sending the link to the client...");

                                send_response(connection, url);

                                verbose(1, "All done.");
                            } else {
                                error("error while making a valid URL.");
                                send_response(connection, "Could not create your paste URL.\nPlease try again later.\n");
                            }
                        } else {
                            error("error while writing paste to disk.");
                            send_response(connection, "Could not write your paste to disk.\nPlease try again later.\n");
                        }
                    } else {
                        error("error while generating a random ID.");
                        send_response(connection, "Could not generate your paste ID.\nPlease try again later.\n");
                    }
                } else {
                    if (errno == EFBIG)
                        send_response(connection, "Paste too big.\n");

                    if (errno == ENOENT)
                        send_response(connection, "Empty paste.\n");

                    if (errno == EAGAIN)
                        send_response(connection, "Timeout'd.\n");

                    error("error %d while reading paste from incoming connection.", errno);
                }

                /* free resources */
                free(paste);
                free(id);
                free(url);

                /* close connection */
                close_connection(connection);
            }

        } else if (pid < 0)
            die(errno, "Could not initialize worker n. %d: %s\n", i, strerror(errno));
    }


    sleep(1);

    verbose(1, "all workers have been initialized.");
    verbose(1, "beginning to accept incoming connections.");

    /* wait for children to finish */
    while(wait(0) > 0);
    close(server);

    return 0;
}

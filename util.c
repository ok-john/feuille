/*
 * util.c
 *  Common utils.
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

#include "util.h"

#ifndef COSMOPOLITAN
#include <stdarg.h>   /* for va_end, va_list, va_start                     */
#include <stdio.h>    /* for snprintf, vsnprintf, vfprintf, BUFSIZ, stderr */
#include <stdlib.h>   /* for exit                                          */
#include <syslog.h>   /* for syslog, LOG_DEBUG, LOG_ERR                    */
#include <unistd.h>   /* for getpid                                        */
#endif

#include "feuille.h"  /* for Settings, settings                            */

/* reuse that buffer for syslogs */
char syslog_buffer[BUFSIZ];

/**
 * Die with an error message.
 *   exit_code: the exit code to be used.
 *   string: the string to be displayed. Printf format is supported.
 */
void die(int exit_code, char *string, ...)
{
    va_list ap;

    va_start(ap, string);
    vfprintf(stderr, string, ap);
    va_end(ap);

    exit(exit_code);
}

/**
 * Send an error message to syslog.
 *   string: the string to be displayed. Printf format is supported.
 */
void error(char *string, ...)
{
    va_list ap;
    va_start(ap, string);

    /* prepend the PID of the current thread */
    int length = snprintf(syslog_buffer, sizeof(syslog_buffer), "ERROR[%d]: ", getpid());

    /* send the string to syslog */
    vsnprintf(syslog_buffer + length, sizeof(syslog_buffer) - length, string, ap);
    syslog(LOG_ERR, "%s", syslog_buffer);

    va_end(ap);
}

/**
 * Send a message to syslog if in verbose mode.
 *   string: the string to be displayed. Printf format is supported.
 */
void verbose(int level, char *string, ...)
{
    if (settings.verbose < level)
        return;

    va_list ap;
    va_start(ap, string);

    /* prepend the PID of the current thread */
    int length = snprintf(syslog_buffer, sizeof(syslog_buffer), "DEBUG%d[%d]: ", level, getpid());

    /* send the string to syslog */
    vsnprintf(syslog_buffer + length, sizeof(syslog_buffer) - length, string, ap);
    syslog(LOG_DEBUG, "%s", syslog_buffer);

    va_end(ap);
}

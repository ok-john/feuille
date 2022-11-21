/*
 * server.c
 *  Server handling.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#include "server.h"

#include <arpa/inet.h>              /* for inet_pton                            */
#include <errno.h>                  /* for errno, EFBIG, ENOENT                 */
#include <netinet/in.h>             /* for sockaddr_in, sockaddr_in6, htons     */
#include <stdio.h>                  /* for puts                                 */
#include <stdlib.h>                 /* for free, NULL, malloc, realloc          */
#include <string.h>                 /* for strcmp, strlen                       */
#include <strings.h>                /* for bzero                                */
#include <syslog.h>                 /* for syslog, LOG_WARNING                  */
#include <sys/socket.h>             /* for setsockopt, bind, socket, AF_INET    */
#include <sys/time.h>               /* for timeval                              */
#include <unistd.h>                 /* for close                                */

#include "feuille.h"                /* for Settings, settings                   */
#include "util.h"                   /* for verbose                              */

/**
 * Initialize the server socket.
 * -> the actual socket.
 */
int initialize_server()
{
    int server;

    /* initialize socket IPv4 and IPv6 addresses */
    verbose(3, "initializing address structs...");

    struct sockaddr_in server_address_v4;
    bzero(&server_address_v4, sizeof(server_address_v4));

    int ipv6_only = 1;
    struct sockaddr_in6 server_address_v6;
    bzero(&server_address_v6, sizeof(server_address_v6));

    if (strcmp(settings.address, "*") == 0) {
        settings.address = "::";
        ipv6_only        = 0;
    }

    /* dirty hack to detect and convert IPv4 / IPv6 addresses */
    verbose(3, "detecting address family...");

    if (inet_pton(AF_INET, settings.address, &server_address_v4.sin_addr) == 1) {
        verbose(3, "IPv4 address detected.");

        /* set socket family and port */
        server_address_v4.sin_family = AF_INET;
        server_address_v4.sin_port   = htons(settings.port);

        /* create socket */
        verbose(3, "creating server socket...");

        if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            return -1;

        /* set socket options */
        verbose(3, "setting socket options...");

        /* reuse address when restarting feuille */
        verbose(3, "  SO_REUSEADDR...");

        if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
            return -1;

        /* bind address and port */
        verbose(3, "binding address on the socket...");

        if (bind(server, (struct sockaddr *)&server_address_v4, sizeof(server_address_v4)) < 0)
            return -1;

    } else if (inet_pton(AF_INET6, settings.address, &server_address_v6.sin6_addr) == 1) {
        verbose(3, "IPv6 address detected.");

        /* set socket family and port */
        server_address_v6.sin6_family = AF_INET6;
        server_address_v6.sin6_port   = htons(settings.port);

        /* create socket */
        verbose(3, "creating server socket...");

        if ((server = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
            return -1;

        /* set socket options */
        verbose(3, "setting socket options...");

        /* reuse address when restarting feuille */
        verbose(3, "  SO_REUSEADDR...");

        if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
            return -1;

        /* Enable dual-stack mode on supported platforms */
        #ifndef __OpenBSD__
            verbose(3, "  IPV6_V6ONLY...");
            if (setsockopt(server, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only)) < 0)
                return -1;
        #else
            if (ipv6_only == 0) {
                puts("");
                syslog(LOG_WARNING, "dual-stack mode is disabled on OpenBSD.");
                syslog(LOG_WARNING, "feuille will only listen on the `::' IPv6 address.");
                puts("");
            }
        #endif

        /* bind address and port */
        verbose(3, "binding address on the socket...");

        if (bind(server, (struct sockaddr *)&server_address_v6, sizeof(server_address_v6)) < 0)
            return -1;

    } else
        return -1;

    /* start listening to incoming connections */
    verbose(3, "starting to listen on the socket...");
    if (listen(server, 1) < 0)
        return -1;

    return server;
}

/**
 * Accept incoming connections.
 *   socket: the server socket.
 * -> the socket associated with the connection.
 */
int accept_connection(int socket)
{
    /* accept the connection */
    /* TODO: maybe retrieve IP address for logging? *maybe* */
    int connection = accept(socket, (struct sockaddr *)NULL, NULL);

    /* set the timeout for the connection */
    struct timeval timeout = { settings.timeout, 0 };

    if (setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        return -1;

    if (setsockopt(connection, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        return -1;

    return connection;
}

/**
 * Close a connection.
 *   connection: the socket associated with the connection.
 */
void close_connection(int connection)
{
    /* prevent reading / writing to the socket */
    shutdown(connection, SHUT_RDWR);

    /* close the socket */
    close(connection);
}

/**
 * Read the incoming data from a connection.
 *   connection: the socket associated with the connection.
 * -> the string containing all data sent, or NULL if an error occured. Needs to be freed.
 */
char *read_paste(int connection)
{
    unsigned long buffer_size = settings.buffer_size;
    unsigned long total_size  = 0;

    /* allocate buffer to store the data */
    char *buffer;
    if ((buffer = malloc((buffer_size + 1) * sizeof(char))) == NULL)
        return NULL;

    /* read all data until EOF is received, or max file size is reached, or the socket timeouts... */
    /* each time, the data is appended to the buffer, once it's been reallocated a larger size */
    long size;
    while ((size = recv(connection, buffer + total_size, buffer_size - total_size, 0)) > 0) {
        total_size += size;

        /* have we reached max file size? */
        if (total_size >= settings.max_size) {
            /* yup, free the buffer and return an error */
            free(buffer);
            errno = EFBIG;
            return NULL;
        }

        /* have we reached the end of the buffer? */
        if (total_size == buffer_size) {
            /* yup, increase the buffer size */
            buffer_size += settings.buffer_size;

            /* reallocate the buffer with a larger size */
            void *tmp;
            if ((tmp = realloc(buffer, (buffer_size + 1) * sizeof(char))) == NULL) {
                free(buffer);
                return NULL;
            }

            buffer = tmp;
        }
    }

    /* is the buffer empty? */
    if (total_size == 0) {
        /* yup, free the buffer and return an error */
        free(buffer);
        errno = ENOENT;
        return NULL;
    }

    /* end the buffer with a null byte */
    buffer[total_size] = 0;
    return buffer;
}

/**
 * Send a response to the client.
 *   connection: the socket associated with the connection.
 *   data: the string to be sent.
 * -> -1 on error, number of bytes sent on success.
 */
int send_response(int connection, char *data) {
    return send(connection, data, strlen(data), 0);
}

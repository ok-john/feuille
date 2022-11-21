/*
 * misc/web/feuille.cgi.c
 *  Auxiliary CGI script that converts an HTTP request to a raw TCP
 *  one.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#include <arpa/inet.h>   /* for inet_pton                                      */
#include <errno.h>       /* for ERANGE, errno                                  */
#include <netinet/in.h>  /* for htons, sockaddr_in, sockaddr_in6               */
#include <stdio.h>       /* for printf, NULL, BUFSIZ                           */
#include <stdlib.h>      /* for calloc, free, getenv, exit, strtoll            */
#include <string.h>      /* for strcmp                                         */
#include <strings.h>     /* for bzero                                          */
#include <sys/socket.h>  /* for connect, socket, AF_INET, AF_INET6, recv, send */
#include <unistd.h>      /* for close, read, STDIN_FILENO                      */

/* Initialize client socket and connect to feuille */
int initialize_socket(char *address, unsigned short port)
{
    int server;

    /* initialize socket IPv4 and IPv6 addresses */
    struct sockaddr_in server_address_v4;
    bzero(&server_address_v4, sizeof(server_address_v4));

    struct sockaddr_in6 server_address_v6;
    bzero(&server_address_v6, sizeof(server_address_v6));

    /* dirty hack to detect and convert IPv4 / IPv6 addresses */
    if (inet_pton(AF_INET, address, &server_address_v4.sin_addr) == 1) {
        /* set socket family and port */
        server_address_v4.sin_family = AF_INET;
        server_address_v4.sin_port   = htons(port);

        /* create socket */
        if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            return -1;

        /* connect to server */
        if (connect(server, (struct sockaddr *)&server_address_v4, sizeof(server_address_v4)) < 0)
            return -1;

    } else if (inet_pton(AF_INET6, address, &server_address_v6.sin6_addr) == 1) {
        /* set socket family and port */
        server_address_v6.sin6_family = AF_INET6;
        server_address_v6.sin6_port   = htons(port);

        /* create socket */
        if ((server = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
            return -1;

        /* connect to server */
        if (connect(server, (struct sockaddr *)&server_address_v6, sizeof(server_address_v6)) < 0)
            return -1;

    } else
        return -1;

    return server;
}

/* CGI helper functions */
void cgi_init()
{
    printf("Content-Type: text/plain\r\n");
}

void cgi_error(char *status)
{
    printf("Status: %s\r\n", status);
    printf("\r\n");
    printf("%s\n", status);
}

void cgi_die(char *status)
{
    cgi_error(status);
    exit(1);
}

void cgi_ok(char *response)
{
    printf("Status: 200 OK\r\n");
    printf("\r\n");
    printf("%s", response);
}

void cgi_redirect(char *url)
{
    printf("Status: 301 Moved Permanently\r\n");
    printf("Location: %s\r\n", url);
    printf("\r\n");
}

/* main function */
int main(void)
{
    cgi_init();

    /* get method */
    char *method;
    if ((method = getenv("REQUEST_METHOD")) == NULL || strcmp(method, "POST") != 0)
        cgi_die("405 Method Not Allowed");

    /* get content length */
    char *content_length;
    if ((content_length = getenv("CONTENT_LENGTH")) == NULL)
        cgi_die("400 Bad Request");

    /* convert content length to a long long int */
    long long length = strtoll(content_length, NULL, 10);
    if (length <= 0 || errno == ERANGE)
        cgi_die("400 Bad Request");

    /* initialize socket */
    int socket;
    if ((socket = initialize_socket(ADDR, PORT)) == -1)
        cgi_die("500 Internal Server Error");

    /* read paste from stdin */
    char *request;
    if ((request = calloc(length + 1, sizeof(char))) == NULL)
        cgi_die("500 Internal Server Error");

    read(STDIN_FILENO, request, length);
    request[length] = 0;

    /* remove `paste=' from request */
    char *paste = request;

    char *offset;
    if ((offset = strstr(request, "=")) != NULL) {
        paste   = offset + 1;
        length -= offset + 1 - request;
    }

    /* send paste to feuille */
    send(socket, paste, length + 1, 0);
    shutdown(socket, SHUT_WR);

    /* receive response from feuille */
    char *response;
    if ((response = calloc(BUFSIZ + 1, sizeof(char))) == NULL)
        cgi_die("500 Internal Server Error");

    if (recv(socket, response, BUFSIZ, 0) > 0) {
        if (strstr(response, "http") != NULL)
            cgi_redirect(response);
        else
            cgi_ok(response);
    } else
        cgi_error("500 Internal Server Error");

    /* close socket, free variables and exit */
    close(socket);
    free(response);
    free(request);

    return 0;
}

/*
 * server.h
 *  server.c header declarations.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include "feuille.h"

int      initialize_server();

int      accept_connection(int);
void     close_connection(int);

char    *read_paste(int);
int      send_response(int, char *);

#endif

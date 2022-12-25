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

#pragma once

#include "feuille.h"

int      initialize_server();

int      accept_connection(int);
void     close_connection(int);

unsigned long   read_paste(int, char **);
int             send_response(int, char *);

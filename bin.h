/*
 * bin.h
 *  bin.c header declarations.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#ifndef _BIN_H_
#define _BIN_H_

#include "feuille.h"

int      paste_exists(char *);
int      write_paste(char *, char *);

char    *generate_id(int);
char    *create_url(char *);

#endif

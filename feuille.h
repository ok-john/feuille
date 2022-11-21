/*
 * feuille.h
 *  feuille.c header declarations.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#ifndef _FEUILLE_H_
#define _FEUILLE_H_

typedef struct Settings {
    char            *address;
    char            *url;
    char            *output;
    char            *user;

    unsigned char    id_length;
    unsigned short   worker_count;
    unsigned short   port;
    unsigned int     timeout;     /* seconds */
    unsigned long    max_size;    /* bytes   */
    unsigned long    buffer_size; /* bytes   */

    char             verbose;
    char             foreground;
} Settings;

extern Settings settings;

#endif

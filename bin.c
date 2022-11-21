/*
 * bin.c
 *  Pastes handling.
 *
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#include "bin.h"

#include <stdio.h>    /* for NULL, fclose, fopen, fputs, snprintf, FILE */
#include <stdlib.h>   /* for calloc, free, malloc, rand, realloc        */
#include <string.h>   /* for strlen                                     */
#include <unistd.h>   /* for access, F_OK                               */

#include "feuille.h"  /* for Settings, settings                         */

/* symbols used to generate IDs */
static char *id_symbols = "abcdefghijklmnopqrstuvwxyz0123456789";

/**
 * Generate a random ID, until one is available on disk.
 *   min_length: the minimum ID length. Will be increased if a collision occurs.
 * -> a pointer to the ID. Needs to be freed.
 */
char *generate_id(int min_length)
{
    int length = min_length;

    /* allocate a buffer to store the ID */
    char *buffer;
    if ((buffer = calloc(length + 1, sizeof(char))) == NULL)
        return NULL;

    /* for each letter, generate a random one */
    for (int i = 0; i < length; i++) {
        if (i > 8 * min_length)
            return NULL;

        buffer[i]     = id_symbols[rand() % strlen(id_symbols)];

        /* collision? */
        if (i == length - 1 && paste_exists(buffer)) {
            /* add one to the ID length and re-allocate the buffer */
            length++;

            void *tmp;
            if ((tmp = realloc(buffer, (length + 1) * sizeof(char))) == NULL) {
                free(buffer);
                return NULL;
            }

            buffer = tmp;
        }
    }

    buffer[length] = 0;
    return buffer;
}

/**
 * Check if an ID is already used.
 *   id: the ID in question.
 * -> 1 if exists, 0 if not.
 */
int paste_exists(char *id)
{
    return access(id, F_OK) == 0;
}

/**
 * Write the paste content to disk.
 *   paste: the string containing the paste.
 *   id: the ID of the paste.
 * -> 0 if done, -1 if not.
 */
int write_paste(char *paste, char *id)
{
    /* open the file with write access */
    FILE *file;
    if ((file = fopen(id, "w")) == NULL)
        return -1;

    /* write the content to file */
    if (fputs(paste, file) == -1) {
        fclose(file);
        return -1;
    }

    /* close the file (obviously) */
    fclose(file);
    return 0;
}

/**
 * Make the full URL for the paste.
 *   id: the ID of the paste.
 * -> a pointer to the URL. Needs to be freed.
 */
char *create_url(char *id)
{
    /* calculate the length of the URL */
    /* the 3 characters added are the trailing slash, newline and null byte */
    int length = strlen(id) + strlen(settings.url) + 3;

    /* allocate a buffer to store the URL */
    char *buffer;
    if ((buffer = malloc(length * sizeof(char))) == NULL)
        return NULL;

    /* set the buffer to the actual URL */
    snprintf(buffer, length, "%s/%s\n", settings.url, id);

    return buffer;
}

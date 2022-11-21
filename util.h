/*
 * util.h
 *  util.c header declarations.
 *
 * Copyright (c) 2022
 *     Tom MTT. <tom@heimdall.pm>
 *
 * This file is licensed under the 3-Clause BSD License.
 * You should have received a copy of the 3-Clause BSD License
 * along with this program. If not, see
 * <https://basedwa.re/tmtt/feuille/src/branch/main/LICENSE>.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

void     die(int, char *, ...);
void     error(char *, ...);
void     verbose(int, char *, ...);

#endif

/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef TELODENDRIA_PARSER_H
#define TELODENDRIA_PARSER_H

#include <stdbool.h>

/***
 * @Nm Parser
 * @Nd Functions for dealing with grammars found in Matrix
 * @Dd November 25 2023
 * @Xr User
 *
 * The
 * .Nm
 * API provides an interface for parsing grammars within the 
 * Matrix specification
 */

/**
 * The host[:port] format in a servername.
 */
typedef struct ServerPart {
    char *hostname;
    char *port;
} ServerPart;
/**
 * A common identifier in the form '&local[:server]', where & determines the 
 * *type* of the identifier.
 */
typedef struct CommonID {
    char sigil;
    char *local;
    ServerPart server;
} CommonID;

/** 
 * Parses a common identifier, as per the Common Identifier Format as defined 
 * by the [matrix] specification.
 */
extern bool ParseCommonID(char *, CommonID *);

/** 
 * Parses the server part in a common identifier.
 */
extern bool ParseServerPart(char *, ServerPart *);

/** 
 * Checks whenever the string is a valid common ID with the correct sigil.
 */
extern bool ValidCommonID(char *, char);

/** 
 * Frees a CommonID's values. Note that it doesn't free the CommonID itself.
 */
extern void CommonIDFree(CommonID);

/** 
 * Frees a ServerPart's values. Note that it doesn't free the ServerPart 
 * itself, and that 
 * .Fn CommonIDFree 
 * automatically deals with its server part.
 */
extern void ServerPartFree(ServerPart);

/**
 * Recompose a Common ID into a string which lives in the heap, and must 
 * therefore be freed with 
 * .Fn Free .
 */
extern char * ParserRecomposeCommonID(CommonID);

/**
 * Recompose a server part into a string which lives in the heap, and must 
 * therefore be freed with 
 * .Fn Free .
 */
extern char * ParserRecomposeServerPart(ServerPart);

/**
 * Compares whenever a ServerName is equivalent to a server name string.
 */
extern bool ParserServerNameEquals(ServerPart, char *);


#endif                             /* TELODENDRIA_PARSER_H */

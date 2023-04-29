/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
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
#ifndef TELODENDRIA_REGTOKEN_H
#define TELODENDRIA_REGTOKEN_H

/**
 * @Nm RegToken
 * @Nd Manage Matrix client registration tokens.
 * @Dd April 29 2023
 * @Xr User
 *
 * .Nm
 * manages registration tokens, which are used to allow specific
 * trusted people to register accounts on a homeserver without
 * necessarily allowing entirely open registration. In Telodendria,
 * registration tokens can also grant privileges to a user upon
 * registration. This allows the initial server administrator account
 * to be granted all privileges when it is created.
 */

#include <Db.h>

/**
 * This structure describes a registration token that is in the
 * database.
 */
typedef struct RegTokenInfo
{
    Db *db;
    DbRef *ref;

    /*
     * The token itself.
     */
    char *name;

    /*
     * Who created this token. Note that this can be NULL if the
     * token was created by Telodendria itself.
     */
    char *owner;

    /*
     * How many times the token was used.
     */
    int used;

    /*
     * How many uses are allowed.
     */
    int uses;

    /*
     * Timestamp when this token was created.
     */
    unsigned long created;

    /*
     * Timestamp when this token expires, or 0 if it does not
     * expire.
     */
    unsigned long expires;

    /*
     * A bit field describing the privileges this token grants. See
     * the User API documentation for the privileges supported.
     */
    int grants;

} RegTokenInfo;

/**
 * ``Use'' the specified registration token by increasing the used
 * count by one and writing the changes to the database. This function
 * only takes action if the token is allowed to be used. Check
 * .Fn RegTokenValid
 * to see if a token actually can be used.
 */
extern void RegTokenUse(RegTokenInfo *);

/**
 * Return a boolean value indicating whether or not the specified
 * token exists in the database.
 */
extern int RegTokenExists(Db *, char *);

/**
 * Delete the specified registration token from the database.
 */
extern int RegTokenDelete(RegTokenInfo *);

/**
 * Retrieve information about the specified registration token from
 * the database.
 */
extern RegTokenInfo * RegTokenGetInfo(Db *, char *);

/**
 * Create a new registration with the given name, owner, expiration
 * timestamp, number of uses, and privileges to grant, all specified
 * in that order. Upon success, a registration token information
 * structure will be returned. Otherwise, NULL will be returned.
 */
extern RegTokenInfo *
RegTokenCreate(Db *, char *, char *, unsigned long, int, int);

/**
 * Free the memory associated with the registration token. This should
 * be called after
 * .Fn RegTokenClose .
 */
extern void RegTokenFree(RegTokenInfo *);

/**
 * Return a boolean value indicating whether or not the specified token
 * is valid. A registration token is only valid if it has not expired
 * and still has at least one remaining use.
 */
extern int RegTokenValid(RegTokenInfo *);

/**
 * Unlock the database reference associated with the given registration
 * token. This should be called before
 * .Fn RegTokenFree .
 */
extern int RegTokenClose(RegTokenInfo *);

#endif

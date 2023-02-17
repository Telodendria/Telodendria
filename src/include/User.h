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
#ifndef TELODENDRIA_USER_H
#define TELODENDRIA_USER_H

#include <Db.h>

typedef struct User User;

typedef struct UserAccessToken
{
    char *user;
    char *string;
    char *deviceId;
    long lifetime;
} UserAccessToken;

typedef struct UserLoginInfo
{
    UserAccessToken *accessToken;
    char *refreshToken;
} UserLoginInfo;

extern int
 UserValidate(char *, char *);

extern int
 UserHistoricalValidate(char *, char *);

extern int
 UserExists(Db *, char *);

extern User *
 UserCreate(Db *, char *, char *);

extern User *
 UserLock(Db *, char *);

extern User *
 UserAuthenticate(Db *, char *);

extern int
 UserUnlock(User *);

extern UserLoginInfo *
 UserLogin(User *, char *, char *, char *, int);

extern char *
 UserGetName(User *);

extern int
 UserCheckPassword(User *, char *);

extern int
 UserSetPassword(User *, char *);

extern int
 UserDeactivate(User *);

extern HashMap *
 UserGetDevices(User *);

extern UserAccessToken *
 UserGenerateAccessToken(User *, char *, int);

extern int
 UserAccessTokenSave(Db *, UserAccessToken *);

#endif                             /* TELODENDRIA_USER_H */

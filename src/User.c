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
#include <User.h>
#include <Util.h>
#include <Memory.h>
#include <Str.h>
#include <Sha2.h>
#include <Json.h>

#include <string.h>

struct User
{
    Db *db;
    DbRef *ref;

    char *name;
};

int
UserValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return 0;
        }

        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              (c == '.') || (c == '_') || (c == '=') || (c == '-') ||
              (c == '/')))
        {
            return 0;
        }

        i++;
    }

    return 1;
}

int
UserHistoricalValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return 0;
        }

        if (!(c >= 0x21 && c <= 0x39) || (c >= 0x3B && c <= 0x7E))
        {
            return 0;
        }

        i++;
    }

    return 1;
}

int
UserExists(Db * db, char *name)
{
    return DbExists(db, 2, "users", name);
}

User *
UserLock(Db * db, char *name)
{
    User *user = NULL;
    DbRef *ref = NULL;

    if (!UserExists(db, name))
    {
        return NULL;
    }

    ref = DbLock(db, 2, "users", name);
    user = Malloc(sizeof(User));
    user->db = db;
    user->ref = ref;
    user->name = StrDuplicate(name);

    return user;
}

int
UserUnlock(User * user)
{
    int ret;

    if (!user)
    {
        return 0;
    }

    Free(user->name);

    ret = DbUnlock(user->db, user->ref);
    Free(user);

    return ret;
}

User *
UserCreate(Db * db, char *name, char *password)
{
    User *user = NULL;
    HashMap *json = NULL;

    char *hash = NULL;
    char *salt = NULL;
    char *tmpstr = NULL;
    unsigned long ts = UtilServerTs();

    /* TODO: Put some sort of password policy(like for example at least
     * 8 chars, or maybe check it's entropy)? */
    if (!db || (name && UserExists(db, name)) || !password || !strlen(password))
    {
        /* User exists or cannot be registered, therefore, do NOT
         * bother */
        return NULL;
    }

    user = Malloc(sizeof(User));
    user->db = db;

    if (!name)
    {
        user->name = StrRandom(12);
    }
    else
    {
        user->name = StrDuplicate(name);
    }

    user->ref = DbCreate(db, 2, "users", user->name);
    if (!user->ref)
    {
        /* The only scenario where I can see that occur is if for some
         * strange reason, Db fails to create a file(e.g fs is full) */
        Free(user->name);
        Free(user);
        return NULL;
    }

    json = DbJson(user->ref);

    /* Generate stored password using a salt and SHA256 */
    salt = StrRandom(16);
    tmpstr = StrConcat(2, password, salt);
    hash = Sha256(tmpstr);
    Free(tmpstr);
    HashMapSet(json, "salt", JsonValueString(salt));
    HashMapSet(json, "hash", JsonValueString(hash));

    HashMapSet(json, "created_on", JsonValueInteger(ts));
    HashMapSet(json, "last_updated", JsonValueInteger(ts));

    return user;
}

void
UserLogin(User * user, char *password, char *deviceId, char *deviceDisplayName)
{
    /* TODO: Implement login */
    (void) user;
    (void) password;
    (void) deviceId;
    (void) deviceDisplayName;
}

char *
UserGetName(User * user)
{
    return user ? user->name : NULL;
}

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

User *
UserAuthenticate(Db * db, char *accessToken)
{
    User *user;
    DbRef *atRef;

    char *userName;
    char *deviceId;
    long expires;

    if (!db || !accessToken)
    {
        return NULL;
    }

    atRef = DbLock(db, 3, "tokens", "access", accessToken);
    if (!atRef)
    {
        return NULL;
    }

    userName = JsonValueAsString(HashMapGet(DbJson(atRef), "user"));
    deviceId = JsonValueAsString(HashMapGet(DbJson(atRef), "device"));
    expires = JsonValueAsInteger(HashMapGet(DbJson(atRef), "expires"));

    user = UserLock(db, userName);
    if (!user)
    {
        DbUnlock(db, atRef);
        return NULL;
    }

    if (expires && UtilServerTs() >= (unsigned long) expires)
    {
        UserUnlock(user);
        DbUnlock(db, atRef);
        return NULL;
    }

    /* TODO: Attach deviceId to User */
    (void) deviceId;

    DbUnlock(db, atRef);
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

    UserSetPassword(user, password);

    json = DbJson(user->ref);
    HashMapSet(json, "createdOn", JsonValueInteger(ts));
    HashMapSet(json, "deactivated", JsonValueBoolean(0));

    return user;
}

UserLoginInfo *
UserLogin(User * user, char *password, char *deviceId, char *deviceDisplayName,
          int withRefresh)
{
    DbRef *rtRef = NULL;

    HashMap *devices;
    HashMap *device;

    UserLoginInfo *result;

    if (!user || !password)
    {
        return NULL;
    }

    if (!UserCheckPassword(user, password))
    {
        return NULL;
    }

    result = Malloc(sizeof(UserLoginInfo));
    if (!result)
    {
        return NULL;
    }

    result->refreshToken = NULL;

    if (!deviceId)
    {
        deviceId = StrRandom(10);
    }
    else
    {
        deviceId = StrDuplicate(deviceId);
    }

    /* Generate an access token */
    result->accessToken = UserGenerateAccessToken(user, deviceId, withRefresh);
    UserAccessTokenSave(user->db, result->accessToken);

    if (withRefresh)
    {
        result->refreshToken = StrRandom(64);
        rtRef = DbCreate(user->db, 3, "tokens", "refresh", result->refreshToken);

        HashMapSet(DbJson(rtRef), "refreshes",
                   JsonValueString(result->accessToken->string));
        DbUnlock(user->db, rtRef);
    }

    devices = JsonValueAsObject(HashMapGet(DbJson(user->ref), "devices"));
    if (!devices)
    {
        devices = HashMapCreate();
        HashMapSet(DbJson(user->ref), "devices", JsonValueObject(devices));
    }

    device = JsonValueAsObject(HashMapGet(devices, deviceId));

    if (device)
    {
        JsonValue *val;

        val = HashMapDelete(device, "accessToken");
        if (val)
        {
            DbDelete(user->db, 3, "tokens", "access", JsonValueAsString(val));
            JsonValueFree(val);
        }

        val = HashMapDelete(device, "refreshToken");
        if (val)
        {
            DbDelete(user->db, 3, "tokens", "refresh", JsonValueAsString(val));
            JsonValueFree(val);
        }
    }
    else
    {
        device = HashMapCreate();
        HashMapSet(devices, deviceId, JsonValueObject(device));

        if (deviceDisplayName)
        {
            HashMapSet(device, "displayName",
                       JsonValueString(deviceDisplayName));
        }

    }

    Free(deviceId);

    if (result->refreshToken)
    {
        HashMapSet(device, "refreshToken",
                   JsonValueString(result->refreshToken));
    }

    HashMapSet(device, "accessToken",
               JsonValueString(result->accessToken->string));

    return result;
}

char *
UserGetName(User * user)
{
    return user ? user->name : NULL;
}

int
UserCheckPassword(User * user, char *password)
{
    HashMap *json;

    char *storedHash;
    char *salt;

    char *hashedPwd;
    char *tmp;

    int result;

    if (!user || !password)
    {
        return 0;
    }

    json = DbJson(user->ref);

    storedHash = JsonValueAsString(HashMapGet(json, "password"));
    salt = JsonValueAsString(HashMapGet(json, "salt"));

    if (!storedHash || !salt)
    {
        return 0;
    }

    tmp = StrConcat(2, password, salt);
    hashedPwd = Sha256(tmp);
    Free(tmp);

    result = strcmp(hashedPwd, storedHash) == 0;

    Free(hashedPwd);

    return result;
}

int
UserSetPassword(User * user, char *password)
{
    HashMap *json;

    char *hash = NULL;
    char *salt = NULL;
    char *tmpstr = NULL;

    if (!user || !password)
    {
        return 0;
    }

    json = DbJson(user->ref);

    salt = StrRandom(16);
    tmpstr = StrConcat(2, password, salt);
    hash = Sha256(tmpstr);

    JsonValueFree(HashMapSet(json, "salt", JsonValueString(salt)));
    JsonValueFree(HashMapSet(json, "password", JsonValueString(hash)));

    Free(salt);
    Free(hash);
    Free(tmpstr);

    return 1;
}

int
UserDeactivate(User * user)
{
    HashMap *json;

    if (!user)
    {
        return 0;
    }

    json = DbJson(user->ref);

    JsonValueFree(HashMapSet(json, "deactivated", JsonValueBoolean(1)));

    return 1;
}

HashMap *
UserGetDevices(User * user)
{
    HashMap *json;

    if (!user)
    {
        return NULL;
    }

    json = DbJson(user->ref);

    return JsonValueAsObject(HashMapGet(json, "devices"));
}

UserAccessToken *
UserGenerateAccessToken(User * user, char *deviceId, int withRefresh)
{
    UserAccessToken *token;

    if (!user || !deviceId)
    {
        return NULL;
    }

    token = Malloc(sizeof(UserAccessToken));
    if (!token)
    {
        return NULL;
    }

    token->user = StrDuplicate(user->name);
    token->deviceId = StrDuplicate(deviceId);

    token->string = StrRandom(64);

    if (withRefresh)
    {
        token->lifetime = 1000 * 60 * 60 * 24 * 7;      /* 1 Week */
    }
    else
    {
        token->lifetime = 0;
    }

    return token;
}

int
UserAccessTokenSave(Db * db, UserAccessToken * token)
{
    DbRef *ref;
    HashMap *json;

    if (!token)
    {
        return 0;
    }

    ref = DbCreate(db, 3, "tokens", "access", token->string);

    if (!ref)
    {
        return 0;
    }

    json = DbJson(ref);

    HashMapSet(json, "user", JsonValueString(token->user));
    HashMapSet(json, "device", JsonValueString(token->deviceId));

    if (token->lifetime)
    {
        HashMapSet(json, "expires", JsonValueInteger(UtilServerTs() + token->lifetime));
    }

    return DbUnlock(db, ref);
}

void
UserAccessTokenFree(UserAccessToken * token)
{
    if (!token)
    {
        return;
    }

    Free(token->user);
    Free(token->string);
    Free(token->deviceId);
    Free(token);
}

int
UserDeleteToken(User * user, char *token)
{
    char *username;
    char *deviceId;
    char *refreshToken;

    Db *db;
    DbRef *tokenRef;

    HashMap *tokenJson;
    HashMap *userJson;
    HashMap *deviceObj;

    JsonValue *deletedVal;

    if (!user || !token)
    {
        return 0;
    }

    db = user->db;
    /* First check if the token even exists */
    if (!DbExists(db, 3, "tokens", "access", token))
    {
        return 0;
    }

    /* If it does, get it's username. */
    tokenRef = DbLock(db, 3, "tokens", "access", token);

    if (!tokenRef)
    {
        return 0;
    }
    tokenJson = DbJson(tokenRef);
    username = JsonValueAsString(HashMapGet(tokenJson, "user"));
    deviceId = JsonValueAsString(HashMapGet(tokenJson, "device"));

    if (strcmp(username, UserGetName(user)) != 0)
    {
        /* Token does not match user, do not delete it */
        DbUnlock(db, tokenRef);
        return 0;
    }

    userJson = DbJson(user->ref);
    deviceObj = JsonValueAsObject(HashMapGet(userJson, "devices"));

    if (!deviceObj)
    {
        return 0;
    }

    /* Delete refresh token, if present */
    refreshToken = JsonValueAsString(JsonGet(deviceObj, 2, deviceId, "refreshToken"));
    if (refreshToken)
    {
        DbDelete(db, 3, "tokens", "refresh", refreshToken);
    }

    /* Delete the device object */
    deletedVal = HashMapDelete(deviceObj, deviceId);
    if (!deletedVal)
    {
        return 0;
    }
    JsonValueFree(deletedVal);

    /* Delete the access token. */
    if (!DbUnlock(db, tokenRef) || !DbDelete(db, 3, "tokens", "access", token))
    {
        return 0;
    }

    return 1;
}

UserId *
UserParseId(char *id, char *defaultServer)
{
    UserId *userId;

    if (!id)
    {
        return NULL;
    }

    id = StrDuplicate(id);
    if (!id)
    {
        return NULL;
    }

    userId = Malloc(sizeof(UserId));
    if (!userId)
    {
        goto finish;
    }

    /* Fully-qualified user ID */
    if (*id == '@')
    {
        char *localStart = id + 1;
        char *serverStart = localStart;

        while (*serverStart != ':' && *serverStart != '\0')
        {
            serverStart++;
        }

        if (*serverStart == '\0')
        {
            Free(userId);
            userId = NULL;
            goto finish;
        }

        *serverStart = '\0';
        serverStart++;

        userId->localpart = StrDuplicate(localStart);
        userId->server = StrDuplicate(serverStart);
    }
    else
    {
        /* Treat it as just a localpart */
        userId->localpart = StrDuplicate(id);
        userId->server = StrDuplicate(defaultServer);
    }

finish:
    Free(id);
    return userId;
}

void 
UserFreeId(UserId *id)
{
    if (id)
    {
        Free(id->localpart);
        Free(id->server);
        Free(id);
    }
}

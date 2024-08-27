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
#include <User.h>
#include <Cytoplasm/Util.h>
#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Sha.h>
#include <Cytoplasm/Json.h>

#include <Parser.h>

#include <string.h>

struct User
{
    Db *db;
    DbRef *ref;

    char *name;
    char *deviceId;
};

bool
UserValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return false;
        }

        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              (c == '.') || (c == '_') || (c == '=') || (c == '-') ||
              (c == '/')))
        {
            return false;
        }

        i++;
    }

    return true;
}

bool
UserHistoricalValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return false;
        }

        if (!((c >= 0x21 && c <= 0x39) || (c >= 0x3B && c <= 0x7E)))
        {
            return false;
        }

        i++;
    }

    return true;
}

bool
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
    user->deviceId = NULL;

    return user;
}

User *
UserAuthenticate(Db * db, char *accessToken)
{
    User *user;
    DbRef *atRef;

    char *userName;
    char *deviceId;
    uint64_t expires;

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
    expires =  JsonValueAsInteger(HashMapGet(DbJson(atRef), "expires"));

    user = UserLock(db, userName);
    if (!user)
    {
        DbUnlock(db, atRef);
        return NULL;
    }

    if (expires && UtilTsMillis() >= expires)
    {
        UserUnlock(user);
        DbUnlock(db, atRef);
        return NULL;
    }

    user->deviceId = StrDuplicate(deviceId);

    DbUnlock(db, atRef);
    return user;
}

bool
UserUnlock(User * user)
{
    bool ret;

    if (!user)
    {
        return false;
    }

    Free(user->name);
    Free(user->deviceId);

    ret = DbUnlock(user->db, user->ref);
    Free(user);

    return ret;
}

User *
UserCreate(Db * db, char *name, char *password)
{
    User *user = NULL;
    HashMap *json = NULL;

    uint64_t ts = UtilTsMillis();

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
    HashMapSet(json, "deactivated", JsonValueBoolean(false));

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

    if (!UserCheckPassword(user, password) || UserDeactivated(user))
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
    result->accessToken = UserAccessTokenGenerate(user, deviceId, withRefresh);
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

char *
UserGetDeviceId(User * user)
{
    return user ? user->deviceId : NULL;
}

bool
UserCheckPassword(User * user, char *password)
{
    HashMap *json;

    char *storedHash;
    char *salt;

    unsigned char *hashBytes;
    char *hashedPwd;
    char *tmp;

    bool result;

    if (!user || !password)
    {
        return false;
    }

    json = DbJson(user->ref);

    storedHash = JsonValueAsString(HashMapGet(json, "password"));
    salt = JsonValueAsString(HashMapGet(json, "salt"));

    if (!storedHash || !salt)
    {
        return false;
    }

    tmp = StrConcat(2, password, salt);
    hashBytes = Sha256(tmp);
    hashedPwd = ShaToHex(hashBytes, HASH_SHA256);
    Free(tmp);
    Free(hashBytes);

    result = StrEquals(hashedPwd, storedHash);

    Free(hashedPwd);

    return result;
}

bool
UserSetPassword(User * user, char *password)
{
    HashMap *json;

    unsigned char *hashBytes;
    char *hash = NULL;
    char *salt = NULL;
    char *tmpstr = NULL;

    if (!user || !password)
    {
        return false;
    }

    json = DbJson(user->ref);

    salt = StrRandom(16);
    tmpstr = StrConcat(2, password, salt);
    hashBytes = Sha256(tmpstr);
    hash = ShaToHex(hashBytes, HASH_SHA256);

    JsonValueFree(HashMapSet(json, "salt", JsonValueString(salt)));
    JsonValueFree(HashMapSet(json, "password", JsonValueString(hash)));

    Free(salt);
    Free(hash);
    Free(hashBytes);
    Free(tmpstr);

    return true;
}

bool
UserDeactivate(User * user, char * from, char * reason)
{
    HashMap *json;
    JsonValue *val;

    if (!user)
    {
        return false;
    }
    
    /* By default, it's the target's username */
    if (!from)
    {
        from = UserGetName(user);
    }

    json = DbJson(user->ref);

    JsonValueFree(HashMapSet(json, "deactivated", JsonValueBoolean(true)));

    val = JsonValueString(from);
    JsonValueFree(JsonSet(json, val, 2, "deactivate", "by"));
    if (reason)
    {
        val = JsonValueString(reason);
        JsonValueFree(JsonSet(json, val, 2, "deactivate", "reason"));
    }

    return true;

}

bool
UserReactivate(User * user)
{
    HashMap *json;

    if (!user)
    {
        return false;
    }

    json = DbJson(user->ref);


    JsonValueFree(HashMapSet(json, "deactivated", JsonValueBoolean(false)));
    
    JsonValueFree(HashMapDelete(json, "deactivate"));

    return true;
}

bool
UserDeactivated(User * user)
{
    HashMap *json;

    if (!user)
    {
        return true;
    }

    json = DbJson(user->ref);

    return JsonValueAsBoolean(HashMapGet(json, "deactivated"));
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
UserAccessTokenGenerate(User * user, char *deviceId, int withRefresh)
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
        token->lifetime = 1000 * 60 * 60 * 24 * 7; /* 1 Week */
    }
    else
    {
        token->lifetime = 0;
    }

    return token;
}

bool
UserAccessTokenSave(Db * db, UserAccessToken * token)
{
    DbRef *ref;
    HashMap *json;

    if (!token)
    {
        return false;
    }

    ref = DbCreate(db, 3, "tokens", "access", token->string);

    if (!ref)
    {
        return false;
    }

    json = DbJson(ref);

    HashMapSet(json, "user", JsonValueString(token->user));
    HashMapSet(json, "device", JsonValueString(token->deviceId));

    if (token->lifetime)
    {
        HashMapSet(json, "expires", JsonValueInteger(UtilTsMillis() + token->lifetime));
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

bool
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
        return false;
    }

    db = user->db;
    /* First check if the token even exists */
    if (!DbExists(db, 3, "tokens", "access", token))
    {
        return false;
    }

    /* If it does, get it's username. */
    tokenRef = DbLock(db, 3, "tokens", "access", token);

    if (!tokenRef)
    {
        return false;
    }
    tokenJson = DbJson(tokenRef);
    username = JsonValueAsString(HashMapGet(tokenJson, "user"));
    deviceId = JsonValueAsString(HashMapGet(tokenJson, "device"));

    if (!StrEquals(username, UserGetName(user)))
    {
        /* Token does not match user, do not delete it */
        DbUnlock(db, tokenRef);
        return false;
    }

    userJson = DbJson(user->ref);
    deviceObj = JsonValueAsObject(HashMapGet(userJson, "devices"));

    if (!deviceObj)
    {
        return false;
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
        return false;
    }
    JsonValueFree(deletedVal);

    /* Delete the access token. */
    if (!DbUnlock(db, tokenRef) || !DbDelete(db, 3, "tokens", "access", token))
    {
        return false;
    }

    return true;
}

char *
UserGetProfile(User * user, char *name)
{
    HashMap *json = NULL;

    if (!user || !name)
    {
        return NULL;
    }

    json = DbJson(user->ref);

    return JsonValueAsString(JsonGet(json, 2, "profile", name));
}

void
UserSetProfile(User * user, char *name, char *val)
{
    HashMap *json = NULL;

    if (!user || !name || !val)
    {
        return;
    }

    json = DbJson(user->ref);
    JsonValueFree(JsonSet(json, JsonValueString(val), 2, "profile", name));
}

bool
UserDeleteTokens(User * user, char *exempt)
{
    HashMap *devices;
    char *deviceId;
    JsonValue *deviceObj;

    if (!user)
    {
        return false;
    }

    devices = JsonValueAsObject(HashMapGet(DbJson(user->ref), "devices"));
    if (!devices)
    {
        return false;
    }

    while (HashMapIterate(devices, &deviceId, (void **) &deviceObj))
    {
        HashMap *device = JsonValueAsObject(deviceObj);
        char *accessToken = JsonValueAsString(HashMapGet(device, "accessToken"));
        char *refreshToken = JsonValueAsString(HashMapGet(device, "refreshToken"));

        if (exempt && (StrEquals(accessToken, exempt)))
        {
            continue;
        }

        if (accessToken)
        {
            DbDelete(user->db, 3, "tokens", "access", accessToken);
        }

        if (refreshToken)
        {
            DbDelete(user->db, 3, "tokens", "refresh", refreshToken);
        }

        JsonValueFree(HashMapDelete(devices, deviceId));
    }

    return true;
}

int
UserGetPrivileges(User * user)
{
    if (!user)
    {
        return USER_NONE;
    }

    return UserDecodePrivileges(JsonValueAsArray(HashMapGet(DbJson(user->ref), "privileges")));
}

bool
UserSetPrivileges(User * user, int privileges)
{
    JsonValue *val;

    if (!user)
    {
        return false;
    }

    if (!privileges)
    {
        JsonValueFree(HashMapDelete(DbJson(user->ref), "privileges"));
        return true;
    }

    val = JsonValueArray(UserEncodePrivileges(privileges));
    if (!val)
    {
        return false;
    }

    JsonValueFree(HashMapSet(DbJson(user->ref), "privileges", val));
    return true;
}

int
UserDecodePrivileges(Array * arr)
{
    int privileges = USER_NONE;

    size_t i;

    if (!arr)
    {
        goto finish;
    }

    for (i = 0; i < ArraySize(arr); i++)
    {
        JsonValue *val = ArrayGet(arr, i);
        if (!val || JsonValueType(val) != JSON_STRING)
        {
            continue;
        }

        privileges |= UserDecodePrivilege(JsonValueAsString(val));
    }

finish:
    return privileges;
}

int
UserDecodePrivilege(const char *p)
{
    if (!p)
    {
        return USER_NONE;
    }
    else if (StrEquals(p, "ALL"))
    {
        return USER_ALL;
    }
    else if (StrEquals(p, "DEACTIVATE"))
    {
        return USER_DEACTIVATE;
    }
    else if (StrEquals(p, "ISSUE_TOKENS"))
    {
        return USER_ISSUE_TOKENS;
    }
    else if (StrEquals(p, "CONFIG"))
    {
        return USER_CONFIG;
    }
    else if (StrEquals(p, "GRANT_PRIVILEGES"))
    {
        return USER_GRANT_PRIVILEGES;
    }
    else if (StrEquals(p, "PROC_CONTROL"))
    {
        return USER_PROC_CONTROL;
    }
    else if (StrEquals(p, "ALIAS"))
    {
        return USER_ALIAS;
    }
    else
    {
        return USER_NONE;
    }
}

Array *
UserEncodePrivileges(int privileges)
{
    Array *arr = ArrayCreate();

    if (!arr)
    {
        return NULL;
    }

    if ((privileges & USER_ALL) == USER_ALL)
    {
        ArrayAdd(arr, JsonValueString("ALL"));
        goto finish;
    }

#define A(priv, as) \
    if ((privileges & priv) == priv) \
    { \
        ArrayAdd(arr, JsonValueString(as)); \
    }

    A(USER_DEACTIVATE, "DEACTIVATE");
    A(USER_ISSUE_TOKENS, "ISSUE_TOKENS");
    A(USER_CONFIG, "CONFIG");
    A(USER_GRANT_PRIVILEGES, "GRANT_PRIVILEGES");
    A(USER_PROC_CONTROL, "PROC_CONTROL");
    A(USER_ALIAS, "ALIAS");

#undef A

finish:
    return arr;
}

CommonID *
UserIdParse(char *id, char *defaultServer)
{
    CommonID *userId;
    char *server;

    if (!id)
    {
        return NULL;
    }

    id = StrDuplicate(id);
    if (!id)
    {
        return NULL;
    }

    userId = Malloc(sizeof(CommonID));
    if (!userId)
    {
        goto finish;
    }
    memset(userId, 0, sizeof(CommonID));

    /* Fully-qualified user ID */
    if (*id == '@')
    {
        if (!ParseCommonID(id, userId) || !userId->server.hostname)
        {
            UserIdFree(userId);

            userId = NULL;
            goto finish;
        }
    }
    else
    {
        /* Treat it as just a localpart */
        userId->local = StrDuplicate(id);
        ParseServerPart(defaultServer, &userId->server);
    }

    server = ParserRecomposeServerPart(userId->server);
    if (!UserHistoricalValidate(userId->local, server))
    {
        UserIdFree(userId);
        userId = NULL;
    }
    Free(server);

finish:
    Free(id);
    return userId;
}

void
UserIdFree(CommonID * id)
{
    if (id)
    {
        CommonIDFree(*id);
        Free(id);
    }
}

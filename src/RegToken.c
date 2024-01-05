/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net>
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
#include <RegToken.h>

#include <string.h>
#include <ctype.h>

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Json.h>
#include <Cytoplasm/Util.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Int64.h>
#include <Cytoplasm/Log.h>

#include <User.h>

int
RegTokenValid(RegTokenInfo * token)
{
    HashMap *tokenJson;
    Int64 uses, used;

    UInt64 expiration;

    if (!token || !RegTokenExists(token->db, token->name))
    {
        return 0;
    }

    tokenJson = DbJson(token->ref);
    uses = JsonValueAsInteger(HashMapGet(tokenJson, "uses"));
    used = JsonValueAsInteger(HashMapGet(tokenJson, "used"));
    expiration = JsonValueAsInteger(HashMapGet(tokenJson, "expires_on"));

    return (UInt64Eq(expiration, UInt64Create(0, 0)) ||
            UInt64Geq(UtilServerTs(), expiration)) &&
            (Int64Eq(uses, Int64Neg(Int64Create(0, 1))) || Int64Lt(used, uses));
}
void
RegTokenUse(RegTokenInfo * token)
{
    HashMap *tokenJson;

    if (!token || !RegTokenExists(token->db, token->name))
    {
        return;
    }

    if (Int64Geq(token->uses, Int64Create(0, 0)) &&
        Int64Geq(token->used, token->uses))
    {
        return;
    }

    token->used = Int64Add(token->used, Int64Create(0, 1));

    /* Write the information to the hashmap */
    tokenJson = DbJson(token->ref);
    JsonValueFree(HashMapSet(tokenJson, "used", JsonValueInteger(token->used)));
}

int
RegTokenExists(Db * db, char *token)
{
    if (!token || !db)
    {
        return 0;
    }
    return DbExists(db, 3, "tokens", "registration", token);
}

int
RegTokenDelete(RegTokenInfo * token)
{
    if (!token || !RegTokenClose(token))
    {
        return 0;
    }
    if (!DbDelete(token->db, 3, "tokens", "registration", token->name))
    {
        return 0;
    }
    RegTokenInfoFree(token);
    Free(token);
    return 1;
}

RegTokenInfo *
RegTokenGetInfo(Db * db, char *token)
{
    RegTokenInfo *ret;

    DbRef *tokenRef;
    HashMap *tokenJson;

    char *errp = NULL;

    if (!RegTokenExists(db, token))
    {
        return NULL;
    }

    tokenRef = DbLock(db, 3, "tokens", "registration", token);
    if (!tokenRef)
    {
        return NULL;
    }
    tokenJson = DbJson(tokenRef);
    ret = Malloc(sizeof(RegTokenInfo));

    if (!RegTokenInfoFromJson(tokenJson, ret, &errp))
    {
        Log(LOG_ERR, "RegTokenGetInfo(): Database decoding error: %s", errp);
        RegTokenFree(ret);
        return NULL;
    }

    ret->db = db;
    ret->ref = tokenRef;

    return ret;
}

void
RegTokenFree(RegTokenInfo *tokeninfo)
{
    if (tokeninfo)
    {
        RegTokenInfoFree(tokeninfo);
        Free(tokeninfo);
    }
}
int
RegTokenClose(RegTokenInfo * tokeninfo)
{
    HashMap *json;

    if (!tokeninfo)
    {
        return 0;
    }

    /* Write object to database. */
    json = RegTokenInfoToJson(tokeninfo);
    DbJsonSet(tokeninfo->ref, json); /* Copies json into internal structure. */
    JsonFree(json);

    return DbUnlock(tokeninfo->db, tokeninfo->ref);
}
static int
RegTokenVerify(char *token)
{
    size_t i, size;
    char c;

    if (!token)
    {
        return 0;
    }
    /* The spec says the following: "The token required for this
     * authentication [...] is an opaque string with maximum length of
     * 64 characters in the range [A-Za-z0-9._~-]." */
    if ((size = strlen(token)) > 64)
    {
        return 0;
    }
    for (i = 0; i < size; i++)
    {
        c = token[i];
        if (!(isalnum(c) || c == '0' || c == '_' || c == '~' || c == '-'))
        {
            return 0;
        }
    }

    return 1;
}

RegTokenInfo *
RegTokenCreate(Db * db, char *name, char *owner, UInt64 expires, Int64 uses, int privileges)
{
    RegTokenInfo *ret;

    UInt64 timestamp = UtilServerTs();

    if (!db || !name)
    {
        return NULL;
    }

    /* -1 indicates infinite uses; zero and all positive values are a
     * valid number of uses; althought zero would be rather useless.
     * Anything less than -1 doesn't make sense. */
    if (Int64Lt(uses, Int64Neg(Int64Create(0, 1))))
    {
        return NULL;
    }

    /* Verify the token */
    if (!RegTokenVerify(name) || (UInt64Gt(expires, UInt64Create(0, 0)) && UInt64Lt(expires, timestamp)))
    {
        return NULL;
    }
    ret = Malloc(sizeof(RegTokenInfo));
    /* Set the token's properties */
    ret->db = db;
    ret->ref = DbCreate(db, 3, "tokens", "registration", name);
    if (!ret->ref)
    {
        /* RegToken already exists or some weird fs error */
        Free(ret);
        return NULL;
    }
    ret->name = StrDuplicate(name);
    ret->created_by = StrDuplicate(owner);
    ret->used = Int64Create(0, 0);
    ret->uses = uses;
    ret->created_on = timestamp;
    ret->expires_on = expires;
    ret->grants = UserEncodePrivileges(privileges);

    return ret;
}

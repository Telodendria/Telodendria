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
#include <UserInteractiveAuth.h>

#include <Json.h>
#include <Util.h>
#include <Matrix.h>

#include <string.h>

static HashMap *
BuildDummyFlow(void)
{
    HashMap *response = HashMapCreate();
    HashMap *dummyFlow = HashMapCreate();
    Array *stages = ArrayCreate();
    Array *flows = ArrayCreate();

    ArrayAdd(stages,
             JsonValueString(UtilStringDuplicate("m.login.dummy")));
    HashMapSet(dummyFlow, "stages", JsonValueArray(stages));
    ArrayAdd(flows, JsonValueObject(dummyFlow));

    HashMapSet(response, "flows", JsonValueArray(flows));
    HashMapSet(response, "params",
               JsonValueObject(HashMapCreate()));

    return response;
}

HashMap *
UserInteractiveAuth(HttpServerContext * context, Db * db,
                    HashMap * request)
{
    JsonValue *auth;
    JsonValue *type;
    JsonValue *session;

    HashMap *authObj;
    char *typeStr;
    char *sessionStr;

    DbRef *ref;

    auth = HashMapGet(request, "auth");
    if (!auth)
    {
        HashMap *response = NULL;
        HashMap *persist;
        char *session = UtilRandomString(24);

        ref = DbLock(db, 1, "user_interactive");
        if (!ref)
        {
            ref = DbCreate(db, 1, "user_interactive");
        }

        persist = DbJson(ref);
        HashMapSet(persist, session, JsonValueNull());
        DbUnlock(db, ref);

        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        response = BuildDummyFlow();

        HashMapSet(response, "session",
                   JsonValueString(UtilStringDuplicate(session)));

        return response;
    }

    if (JsonValueType(auth) != JSON_OBJECT)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_BAD_JSON);
    }

    authObj = JsonValueAsObject(auth);
    type = HashMapGet(authObj, "type");
    session = HashMapGet(authObj, "session");

    if (!type || JsonValueType(type) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_BAD_JSON);
    }

    if (!session || JsonValueType(session) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        return BuildDummyFlow();
    }

    typeStr = JsonValueAsString(session);
    sessionStr = JsonValueAsString(session);

    if (strcmp(typeStr, "m.login.dummy") != 0)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_INVALID_PARAM);
    }

    ref = DbLock(db, 1, "user_interactive");

    /* Check to see if session exists */
    if (!ref || !HashMapGet(DbJson(ref), sessionStr))
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        return MatrixErrorCreate(M_UNKNOWN);
    }

    /* We only need to know that it exists. */
    DbUnlock(db, ref);

    return NULL;                   /* All good, auth successful */
}

void
UserInteractiveAuthCleanup(MatrixHttpHandlerArgs * args)
{
    Log(args->lc, LOG_DEBUG, "Purging old user interactive auth sessions...");
    if (!DbDelete(args->db, 1, "user_interactive"))
    {
        Log(args->lc, LOG_ERR, "Failed to purge user_interactive.");
    }
}

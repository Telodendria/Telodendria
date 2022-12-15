/*
 * Copyright (C) 2022 Jordan Bancino <@jordan:bancino.net>
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
#include <Routes.h>

#include <string.h>

#include <Json.h>
#include <HashMap.h>
#include <Util.h>
#include <Memory.h>

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

ROUTE_IMPL(RouteRegister, args)
{
    HashMap *request = NULL;
    HashMap *response = NULL;

    char *pathPart = NULL;

    DbRef *ref;
    HashMap *persist;

    if (MATRIX_PATH_PARTS(args->path) == 0)
    {
        JsonValue *auth = NULL;

        HashMap *authObj = JsonValueAsObject(auth);
        JsonValue *type = HashMapGet(authObj, "type");
        JsonValue *session = HashMapGet(authObj, "session");

        char *typeStr;
        char *sessionStr;

        if (HttpRequestMethodGet(args->context) != HTTP_POST)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_UNRECOGNIZED);
        }

        if (!(args->matrixArgs->config->flags & TELODENDRIA_REGISTRATION))
        {
            HttpResponseStatus(args->context, HTTP_FORBIDDEN);
            return MatrixErrorCreate(M_FORBIDDEN);
        }

        request = JsonDecode(HttpStream(args->context));
        if (!request)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            return MatrixErrorCreate(M_NOT_JSON);
        }

        auth = HashMapGet(request, "auth");
        if (!auth)
        {
            char *session = UtilRandomString(24);

            ref = DbCreate(args->matrixArgs->db, 2,
                                  "user_interactive", session);
            persist = DbJson(ref);

            HashMapSet(persist, "created",
                       JsonValueInteger(UtilServerTs()));
            HashMapSet(persist, "completed", JsonValueBoolean(0));

            DbUnlock(args->matrixArgs->db, ref);

            HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
            response = BuildDummyFlow();

            HashMapSet(response, "session", JsonValueString(session));

            goto finish;
        }

        if (JsonValueType(auth) != JSON_OBJECT)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON);
            goto finish;
        }

        if (!type || JsonValueType(type) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_BAD_JSON);
            goto finish;
        }

        if (!session || JsonValueType(session) != JSON_STRING)
        {
            HttpResponseStatus(args->context, HTTP_UNAUTHORIZED);
            response = BuildDummyFlow();
            goto finish;
        }

        typeStr = JsonValueAsString(session);
        sessionStr = JsonValueAsString(session);

        if (strcmp(typeStr, "m.login.dummy") != 0)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_INVALID_PARAM);
            goto finish;
        }

        /* Check to see if session exists */
        ref = DbLock(args->matrixArgs->db, 2,
            "user_interactive", sessionStr);
        
        if (!ref)
        {
            HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
            response = MatrixErrorCreate(M_UNKNOWN);
            goto finish;
        }

        /* We only need to know that it exists. */
        DbUnlock(args->matrixArgs->db, ref);
        DbDelete(args->matrixArgs->db, 2,
            "user_interactive", sessionStr);
        
        /* TODO: Abstract all the above logic out to a function */
        /* TODO: Register new user here */

finish:
        JsonFree(request);
    }
    else
    {
        pathPart = MATRIX_PATH_POP(args->path);

        if (HttpRequestMethodGet(args->context) == HTTP_GET &&
            MATRIX_PATH_EQUALS(pathPart, "available"))
        {
            char *username = HashMapGet(
                        HttpRequestParams(args->context), "username");

            if (!username)
            {
                HttpResponseStatus(args->context, HTTP_BAD_REQUEST);
                response = MatrixErrorCreate(M_MISSING_PARAM);
            }

            /* TODO: Check if username is available */
        }
        else if (HttpRequestMethodGet(args->context) == HTTP_POST &&
                 (MATRIX_PATH_EQUALS(pathPart, "email") ||
                  MATRIX_PATH_EQUALS(pathPart, "msisdn")))
        {
            Free(pathPart);
            pathPart = MATRIX_PATH_POP(args->path);
            if (!MATRIX_PATH_EQUALS(pathPart, "requestToken"))
            {
                HttpResponseStatus(args->context, HTTP_NOT_FOUND);
                response = MatrixErrorCreate(M_UNRECOGNIZED);
            }
            else
            {
                /* TODO: Validate request body and potentially return
                 * M_BAD_JSON */
                HttpResponseStatus(args->context, HTTP_FORBIDDEN);
                response = MatrixErrorCreate(M_THREEPID_DENIED);
            }
        }
        else
        {
            HttpResponseStatus(args->context, HTTP_NOT_FOUND);
            response = MatrixErrorCreate(M_UNRECOGNIZED);
        }

        Free(pathPart);
    }

    return response;
}

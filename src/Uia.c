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
#include <Uia.h>

#include <string.h>

#include <Memory.h>
#include <Array.h>
#include <Json.h>
#include <Str.h>

#include <Matrix.h>

struct UiaStage
{
    char *type;
    HashMap *params;
};

static HashMap *
BuildFlows(Array * flows)
{
    HashMap *response;
    Array *responseFlows;
    HashMap *responseParams;

    size_t i, j;

    if (!flows)
    {
        return NULL;
    }

    response = HashMapCreate();
    if (!response)
    {
        return NULL;
    }

    responseFlows = ArrayCreate();
    if (!responseFlows)
    {
        HashMapFree(response);
        return NULL;
    }

    responseParams = HashMapCreate();
    if (!responseParams)
    {
        HashMapFree(response);
        ArrayFree(responseFlows);
        return NULL;
    }

    HashMapSet(response, "flows", JsonValueArray(responseFlows));
    HashMapSet(response, "params", JsonValueObject(responseParams));

    for (i = 0; i < ArraySize(flows); i++)
    {
        Array *stages = ArrayGet(flows, i);
        HashMap *responseFlow = HashMapCreate();
        Array *responseStages = ArrayCreate();

        HashMapSet(responseFlow, "stages", JsonValueArray(responseStages));
        ArrayAdd(responseFlows, JsonValueObject(responseFlow));

        for (j = 0; j < ArraySize(stages); j++)
        {
            UiaStage *stage = ArrayGet(stages, i);

            ArrayAdd(responseStages, JsonValueString(stage->type));
            if (stage->params)
            {
                JsonValueFree(HashMapSet(responseParams, stage->type, JsonValueObject(stage->params)));
            }
        }
    }

    return response;
}

static int
BuildResponse(Array * flows, char *session, Db * db, HashMap ** response)
{
    DbRef *ref;
    HashMap *json;

    *response = BuildFlows(flows);

    if (!*response)
    {
        return -1;
    }

    if (!session)
    {
        session = StrRandom(16);
        if (!session)
        {
            JsonFree(*response);
            return -1;
        }

        ref = DbCreate(db, 2, "user_interactive", session);
        if (!ref)
        {
            Free(session);
            JsonFree(*response);
            return -1;
        }

        json = DbJson(ref);
        HashMapSet(json, "completed", JsonValueArray(ArrayCreate()));
        DbUnlock(db, ref);

        HashMapSet(*response, "completed", JsonValueArray(ArrayCreate()));
    }
    else
    {
        Array *completed = ArrayCreate();
        Array *dbCompleted;
        size_t i;

        if (!completed)
        {
            JsonFree(*response);
            return -1;
        }

        ref = DbLock(db, 2, "user_interactive", session);
        if (!ref)
        {
            JsonFree(*response);
            ArrayFree(completed);
            return -1;
        }

        json = DbJson(ref);
        dbCompleted = JsonValueAsArray(HashMapGet(json, "completed"));

        for (i = 0; i < ArraySize(dbCompleted); i++)
        {
            char *stage = JsonValueAsString(ArrayGet(dbCompleted, i));

            ArrayAdd(completed, JsonValueString(stage));
        }

        HashMapSet(*response, "completed", JsonValueArray(completed));

        DbUnlock(db, ref);
    }

    HashMapSet(*response, "session", JsonValueString(session));
    return 0;
}

Array *
UiaDummyFlow(void)
{
    Array *response = ArrayCreate();

    if (!response)
    {
        return NULL;
    }

    ArrayAdd(response, UiaBuildStage("m.login.dummy", NULL));

    return response;
}

UiaStage *
UiaBuildStage(char *type, HashMap * params)
{
    UiaStage *stage = Malloc(sizeof(UiaStage));

    if (!stage)
    {
        return NULL;
    }

    stage->type = type;
    stage->params = params;

    return stage;
}

int
UiaComplete(Array * flows, HttpServerContext * context, Db * db,
            HashMap * request, HashMap ** response)
{
    JsonValue *val;
    HashMap *auth;
    char *session;
    char *authType;

    DbRef *dbRef;
    HashMap *dbJson;

    size_t i, j;
    int ret = 0;

    if (!flows)
    {
        return -1;
    }

    if (!context || !db || !request || !response)
    {
        ret = -1;
        goto finish;
    }

    val = HashMapGet(request, "auth");

    if (!val)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, NULL, db, response);
        goto finish;
    }

    if (JsonValueType(val) != JSON_OBJECT)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        ret = 0;
        goto finish;
    }

    auth = JsonValueAsObject(val);
    val = HashMapGet(auth, "session");

    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        ret = 0;
        goto finish;
    }

    session = JsonValueAsString(val);
    val = HashMapGet(auth, "type");

    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        ret = 0;
        goto finish;
    }

    authType = JsonValueAsString(val);

    dbRef = DbLock(db, 2, "user_interactive", session);
    if (!dbRef)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, StrDuplicate(session), db, response);
        goto finish;
    }

    dbJson = DbJson(dbRef);

    DbUnlock(db, dbRef);

    ret = 1;

finish:
    for (i = 0; i < ArraySize(flows); i++)
    {
        Array *stages = ArrayGet(flows, i);

        for (j = 0; j < ArraySize(stages); j++)
        {
            UiaStage *stage = ArrayGet(stages, j);

            Free(stage->type);
            /* stage->params, if not null, is referenced in the
             * response body. */
            Free(stage);
        }
        ArrayFree(stages);
    }
    ArrayFree(flows);
    return ret;
}

void
UiaCleanup(MatrixHttpHandlerArgs * args)
{
    Log(args->lc, LOG_DEBUG, "Purging old user interactive auth sessions...");
    if (!DbDelete(args->db, 1, "user_interactive"))
    {
        Log(args->lc, LOG_ERR, "Failed to purge user_interactive.");
    }
}

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
BuildResponse(Array * flows, Db * db, HashMap ** response, char *session, DbRef * ref)
{
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

        json = DbJson(ref);
        dbCompleted = JsonValueAsArray(HashMapGet(json, "completed"));

        for (i = 0; i < ArraySize(dbCompleted); i++)
        {
            char *stage = JsonValueAsString(ArrayGet(dbCompleted, i));

            ArrayAdd(completed, JsonValueString(stage));
        }

        HashMapSet(*response, "completed", JsonValueArray(completed));

        session = StrDuplicate(session);
    }

    HashMapSet(*response, "session", JsonValueString(session));
    Free(session);

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

    stage->type = StrDuplicate(type);
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
    Array *completed;
    Array *possibleNext;
    size_t i;

    DbRef *dbRef;
    HashMap *dbJson;
    int ret;

    if (!flows)
    {
        return -1;
    }

    if (!context || !db || !request || !response)
    {
        return -1;
    }

    val = HashMapGet(request, "auth");

    if (!val)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        return BuildResponse(flows, db, response, NULL, NULL);
    }

    if (JsonValueType(val) != JSON_OBJECT)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        return 0;
    }

    auth = JsonValueAsObject(val);
    val = HashMapGet(auth, "session");

    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        return 0;
    }

    session = JsonValueAsString(val);

    dbRef = DbLock(db, 2, "user_interactive", session);
    if (!dbRef)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        return BuildResponse(flows, db, response, NULL, NULL);
    }

    dbJson = DbJson(dbRef);

    completed = JsonValueAsArray(HashMapGet(dbJson, "completed"));
    possibleNext = ArrayCreate();

    for (i = 0; i < ArraySize(flows); i++)
    {
        size_t j;

        Array *stages = ArrayGet(flows, i);

        if (ArraySize(stages) > ArraySize(completed))
        {
            UiaStage *stage = ArrayGet(stages, ArraySize(completed));

            ArrayAdd(possibleNext, stage->type);
        }
        else if (ArraySize(stages) == ArraySize(completed))
        {
            for (j = 0; j < ArraySize(stages); j++)
            {
                UiaStage *stage = ArrayGet(stages, j);
                char *flowStage = stage->type;
                char *completedStage = JsonValueAsString(ArrayGet(completed, j));

                if (strcmp(flowStage, completedStage) != 0)
                {
                    break;
                }
            }

            if (j == ArraySize(stages))
            {
                /* Success: completed matches a stage perfectly */
                ret = 1;
                goto finish;
            }
        }
    }

    val = HashMapGet(auth, "type");

    if (!val || JsonValueType(val) != JSON_STRING)
    {
        HttpResponseStatus(context, HTTP_BAD_REQUEST);
        *response = MatrixErrorCreate(M_BAD_JSON);
        ret = 0;
        goto finish;
    }

    authType = JsonValueAsString(val);

    for (i = 0; i < ArraySize(possibleNext); i++)
    {
        char *possible = ArrayGet(possibleNext, i);

        if (strcmp(authType, possible) == 0)
        {
            break;
        }
    }

    if (i == ArraySize(possibleNext))
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, db, response, session, dbRef);
        goto finish;
    }

    if (strcmp(authType, "m.login.dummy") == 0)
    {
        /* Do nothing */
    }
    else if (strcmp(authType, "m.login.password") == 0)
    {
        /* TODO */
    }
    else if (strcmp(authType, "m.login.registration_token") == 0)
    {
        /* TODO */
    }
    else if (strcmp(authType, "m.login.recaptcha") == 0)
    {
        /* TODO */
    }
    else if (strcmp(authType, "m.login.sso") == 0)
    {
        /* TODO */
    }
    else if (strcmp(authType, "m.login.email.identity") == 0)
    {
        /* TODO */
    }
    else if (strcmp(authType, "m.login.msisdn") == 0)
    {
        /* TODO */
    }
    else
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, db, response, session, dbRef);
        goto finish;
    }

    ArrayAdd(completed, JsonValueString(authType));

    ret = 1;

finish:
    ArrayFree(possibleNext);
    DbUnlock(db, dbRef);
    return ret;
}

void
UiaFlowsFree(Array * flows)
{
    size_t i, j;

    if (!flows)
    {
        return;
    }

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
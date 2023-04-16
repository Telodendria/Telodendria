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

#include <RegToken.h>
#include <Memory.h>
#include <Array.h>
#include <Json.h>
#include <Str.h>
#include <Util.h>

#include <Matrix.h>
#include <User.h>

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
            UiaStage *stage = ArrayGet(stages, j);

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
        HashMapSet(json, "last_access", JsonValueInteger(UtilServerTs()));
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

    ArrayAdd(response, UiaStageBuild("m.login.dummy", NULL));

    return response;
}

UiaStage *
UiaStageBuild(char *type, HashMap * params)
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
            HashMap * request, HashMap ** response, Config * config)
{
    JsonValue *val;
    HashMap *auth;
    char *session;
    char *authType;
    Array *completed;
    Array *possibleNext;
    int remaining[16];             /* There should never be more than
                                    * this many stages in a flow,
                                    * right? */
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
            remaining[ArraySize(possibleNext) - 1] = ArraySize(stages) - ArraySize(completed);
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
        char *password = JsonValueAsString(HashMapGet(auth, "password"));
        HashMap *identifier = JsonValueAsObject(HashMapGet(auth, "identifier"));
        char *type;
        UserId *userId;
        User *user;

        if (!password || !identifier)
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            goto finish;
        }

        type = JsonValueAsString(HashMapGet(identifier, "type"));
        userId = UserIdParse(JsonValueAsString(HashMapGet(identifier, "user")),
                             config->serverName);

        if (!type || strcmp(type, "m.id.user") != 0
        || !userId || strcmp(userId->server, config->serverName) != 0)
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            UserIdFree(userId);
            goto finish;
        }

        user = UserLock(db, userId->localpart);
        if (!user)
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            UserIdFree(userId);
            goto finish;
        }

        if (!UserCheckPassword(user, password))
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            UserIdFree(userId);
            UserUnlock(user);
            goto finish;
        }

        UserIdFree(userId);
        UserUnlock(user);
    }
    else if (strcmp(authType, "m.login.registration_token") == 0)
    {
        RegTokenInfo *tokenInfo;

        char *token = JsonValueAsString(HashMapGet(auth, "token"));

        if (!RegTokenExists(db, token))
        {
            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            goto finish;
        }
        tokenInfo = RegTokenGetInfo(db, token);
        if (!RegTokenValid(tokenInfo))
        {
            RegTokenClose(tokenInfo);
            RegTokenFree(tokenInfo);

            HttpResponseStatus(context, HTTP_UNAUTHORIZED);
            ret = BuildResponse(flows, db, response, session, dbRef);
            goto finish;
        }
        /* Use the token, and then close it. */
        RegTokenUse(tokenInfo);
        RegTokenClose(tokenInfo);
        RegTokenFree(tokenInfo);

        /*
         * Drop the registration token into the session storage because
         * the registration endpoint will have to extract the proper
         * privileges to set on the user based on the token.
         */
        JsonValueFree(HashMapSet(dbJson, "registration_token", JsonValueString(token)));
    }
    /* TODO: implement m.login.recaptcha, m.login.sso,
     * m.login.email.identity, m.login.msisdn here */
    else
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, db, response, session, dbRef);
        goto finish;
    }

    ArrayAdd(completed, JsonValueString(authType));

    if (remaining[i] - 1 > 0)
    {
        HttpResponseStatus(context, HTTP_UNAUTHORIZED);
        ret = BuildResponse(flows, db, response, session, dbRef);
        goto finish;
    }

    ret = 1;

finish:
    ArrayFree(possibleNext);
    JsonValueFree(HashMapSet(dbJson, "last_access", JsonValueInteger(UtilServerTs())));
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
    Array *sessions = DbList(args->db, 1, "user_interactive");
    size_t i;

    Log(LOG_DEBUG, "User Interactive Auth sessions: %lu",
        ArraySize(sessions));
    for (i = 0; i < ArraySize(sessions); i++)
    {
        char *session = ArrayGet(sessions, i);
        DbRef *ref = DbLock(args->db, 2, "user_interactive", session);

        unsigned long lastAccess;

        if (!ref)
        {
            Log(LOG_ERR, "Unable to lock uia %s for inspection.",
                session);
            continue;
        }

        lastAccess = JsonValueAsInteger(HashMapGet(DbJson(ref), "last_access"));

        /* If last access was greater than 15 minutes ago, remove this
         * session */
        if (UtilServerTs() - lastAccess > 1000 * 60 * 15)
        {
            DbUnlock(args->db, ref);
            DbDelete(args->db, 2, "user_interactive", session);
            Log(LOG_DEBUG, "Deleted session %s", session);
        }

        DbUnlock(args->db, ref);
    }

    DbListFree(sessions);
}

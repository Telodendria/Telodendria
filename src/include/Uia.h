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
#ifndef TELODENDRIA_UIA_H
#define TELODENDRIA_UIA_H

/***
 * @Nm Uia
 * @Nd User Interactive Authentication.
 * @Dd April 28 2023
 * @Xr User Db Cron
 *
 * .Nm
 * takes care of all the logic for performing user interactive
 * authentication as defined by the Matrix specification. API endpoints
 * that require authentication via user interactive authentication
 * build up flows and add any necessary parameters, and pass them all
 * into
 * .Fn UiaComplete .
 * The goal is to make it easy for the numerous API endpoints that
 * utilize this authentication mechanism to implement it.
 */

#include <Cytoplasm/Array.h>
#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/HttpServer.h>
#include <Matrix.h>

/**
 * An opaque structure that represents a single stage, which consists
 * of the type and a JSON object that contains implementation-specific
 * parameters for completing the stage.
 */
typedef struct UiaStage UiaStage;

/**
 * Build a single stage with the type and a JSON object of parameters
 * the client may require to complete it. Consult the Matrix
 * specification for the valid types.
 */
extern UiaStage * UiaStageBuild(char *, HashMap *);

/**
 * Build a flow that consists only of a dummy stage. This is useful
 * when an endpoint is required by the specification to use user
 * interactive authentication, but doesn't want to actually require
 * the user to do anything. Since the dummy flow is a fairly common
 * flow, it seemed sensible to have a function for it. Other flows are
 * built manually by the caller that that wishes to perform user
 * interactive authentication.
 */
extern Array * UiaDummyFlow(void);

/**
 * This function should be called periodically to purge old sessions.
 * Sessions are only valid for a few minutes after their last access.
 * After that, they should be purged so that the database doesn't fill
 * up with old session files. This function is specifically designed
 * to be called via the Cron API.
 */
extern void UiaCleanup(MatrixHttpHandlerArgs *);

/**
 * Validate an auth object and maintain session state to track the
 * progress of a client through user interactive authentication flows.
 * The idea is that an API endpoint will not progress until user
 * interactive authentication has succeeded.
 * .Pp
 * This function does the bulk of the work for user interactive
 * authentication. It takes many parameters:
 * .Bl -bullet -offset indent
 * .It
 * An array of arrays of stages. Stages should be created with
 * .Fn UiaStageBuild
 * and then put into an array to create a flow. Those flows should then
 * be put into an array and passed as this parameter. It is important
 * to note here that because of the loose typing of the Array API, it
 * is very easy to make mistakes here; if you are implementing a new
 * endpoint that requires user interactive authentication, it is best
 * to refer to the source code of an existing endpoint to get a better
 * idea of how it works.
 * .It
 * An HTTP server context. This is required to set the response headers
 * in the even of an error.
 * .It
 * The database where user interactive authentication sessions are
 * persisted.
 * .It
 * The JSON request body that contains the client's auth object, which
 * will be read, parsed, and handled as appropriate.
 * .It
 * A pointer to a pointer where a JSON response can be placed if
 * necessary. If this function encounters a client error, such as a
 * failure to authenticate, or outstanding stages that have not yet
 * been completed, it will place a JSON response here that is expected
 * to be returned to the client. This response will include a
 * description of all the flows, stages, and the stage parameters.
 * .It
 * A valid configuration structure, because a few values are read from
 * the configuration during certain stages of authentication.
 * .El
 * .Pp
 * This function returns an integer value less than zero if it
 * experiences an internal failure, such as a failure to allocate
 * memory memory, or a corrupted database. It returns 0 if the client
 * has remaining stages to complete, including the current stage if
 * that one did not complete successfully. In this case, this function
 * will set the proper response headers and the passed response
 * pointer, so the caller should immediately return the response to
 * the client. This function returns 1 if and only if the client has
 * successfully completed all stages. Only in this latter case shall
 * the caller proceed with its logic.
 */
extern int
 UiaComplete(Array *, HttpServerContext *, Db *, HashMap *, HashMap **, Config *);

/**
 * Free an array of flows, as described above. Even though the caller
 * constructs this array, it is convenient to free it in its
 * entirety in a single function call.
 */
extern void UiaFlowsFree(Array *);

#endif

/*
 * Copyright (C) 2022-2025 Jordan Bancino <@jordan:synapse.telodendria.org>
 * with other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

#include <Cytoplasm/Json.h>
#include <Cytoplasm/Array.h>
#include <Cytoplasm/HashMap.h>

ROUTE_IMPL(RouteVersions, path, argp)
{
    HashMap *response = HashMapCreate();
    Array *versions = ArrayCreate();

    (void) path;
    (void) argp;

#define DECLARE_SPEC_VERSION(x) ArrayAdd(versions, JsonValueString(x))
    DECLARE_SPEC_VERSION("v1.2");
    DECLARE_SPEC_VERSION("v1.3");
    DECLARE_SPEC_VERSION("v1.4");
    DECLARE_SPEC_VERSION("v1.5");
    DECLARE_SPEC_VERSION("v1.6");

    /* The curently supported version. */
    DECLARE_SPEC_VERSION("v1.7");

#undef DECLARE_SPEC_VERSION

    HashMapSet(response, "versions", JsonValueArray(versions));
    return response;
}

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
#include <Routes.h>

#include <string.h>

#include <Memory.h>
#include <Json.h>
#include <HashMap.h>
#include <Str.h>

ROUTE_IMPL(RouteCapabilities, path, argp)
{
    HashMap *response;
    HashMap *capabilities;

    (void) path;
    (void) argp;

    response = HashMapCreate();
    capabilities = HashMapCreate();

    JsonSet(capabilities, JsonValueBoolean(1), 2, "m.change_password", "enabled");
    JsonSet(capabilities, JsonValueBoolean(1), 2, "m.set_displayname", "enabled");
    JsonSet(capabilities, JsonValueBoolean(1), 2, "m.set_avatar_url", "enabled");
    JsonSet(capabilities, JsonValueBoolean(0), 2, "m.3pid_changes", "enabled");

    HashMapSet(response, "capabilities", JsonValueObject(capabilities));
    return response;
}

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
#include <Uri.h>

#include <stdio.h>
#include <string.h>

#include <Memory.h>

Uri *
UriParse(const char *str)
{
    Uri *uri;
    int res;

    if (!str)
    {
        return NULL;
    }

    uri = Malloc(sizeof(Uri));
    if (!uri)
    {
        return NULL;
    }

    memset(uri, 0, sizeof(Uri));

    res = sscanf(str, "%7[^:]://%127[^:]:%hu%255[^\n]", uri->proto, uri->host, &uri->port, uri->path) == 4
            || sscanf(str, "%7[^:]://%127[^/]%255[^\n]", uri->proto, uri->host, uri->path) == 3
            || sscanf(str, "%7[^:]://%127[^:]:%hu[^\n]", uri->proto, uri->host, &uri->port) == 3
            || sscanf(str, "%7[^:]://%127[^\n]", uri->proto, uri->host) == 2;

    if (!res)
    {
        Free(uri);
        return NULL;
    }

    if (!uri->path[0])
    {
        strcpy(uri->path, "/");
    }

    return uri;
}

void
UriFree(Uri * uri)
{
    Free(uri);
}

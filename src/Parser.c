/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net> with
 * other valuable contributors. See CONTRIBUTORS.txt for the full list.
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

#include <Parser.h>

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Int.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/* Iterate through a char **. */
#define Iterate(s) (*(*s)++)

/* Parse an extended localpart */
static int
ParseUserLocalpart(char **str, char **out)
{
    char c;
    char *start;
    size_t length;

    if (!str || !out)
    {
        return 0;
    }
    /* An extended localpart contains every ASCII printable character, 
     * except an ':'. */
    start = *str;
    while (isascii((c = Iterate(str))) && c != ':' && c)
    {
        /* Do nothing */
    }
    length = (size_t) (*str - start) - 1;
    if (length < 1)
    {
        *str = start;
        return 0;
    }
    if (c == ':')
    {
        --(*str);
    }

    *out = Malloc(length + 1);
    memcpy(*out, start, length);
    (*out)[length] = '\0';

    return 1;
}
/* Parses an IPv4 address. */
static int
ParseIPv4(char **str, char **out)
{
    /* Be *very* careful with this buffer */
    char buffer[4];
    char *start;
    size_t length;
    char c;

    int digit = 0;
    int digits = 0;

    memset(buffer, 0, sizeof(buffer));
    start = *str;

    /* An IPv4 address is made of 4 blocks between 1-3 digits, like so:
     * (1-3)*DIGIT.(1-3)*DIGIT.(1-3)*DIGIT.(1-3)*DIGIT */
    while ((isdigit(c = Iterate(str)) || c == '.') && c && digits < 4)
    {
        if (isdigit(c))
        {
            digit++;
            continue;
        }
        if (digit < 1 || digit > 3)
        {
            /* Current digit is too long for the spec! */
            *str = start;
            return 0;
        }
        memcpy(buffer, *str - digit - 1, digit);
        if (atoi(buffer) > 255)
        {
            /* Current digit is too large for the spec! */
            *str = start;
            return 0;
        }
        memset(buffer, 0, sizeof(buffer));
        digit = 0;
        digits++; /* We have parsed a digit. */
    }
    if (c == '.' || digits != 3)
    {
        *str = start;
        return 0;
    }
    length = (size_t) (*str - start) - 1;
    *out = Malloc(length + 1);
    memcpy(*out, start, length);
    (*str)--;
    return 1;
}
static int
IsIPv6Char(char c)
{
    return isxdigit(c) || c == ':' || c == '.';
}
static int
ParseIPv6(char **str, char **out)
{
    char *start;
    size_t length;
    char c;

    int filled = 0;
    int digit = 0;
    int digits = 0;

    start = *str;
    length = 0;

    if (Iterate(str) != '[')
    {
        goto fail;
    }

    while ((c = Iterate(str)) && IsIPv6Char(c))
    {
        char *ipv4;
        if (isxdigit(c))
        {
            digit++;
            length++;
            continue;
        }
        if (c == ':')
        {
            if (**str == ':')
            {
                digit = 0;
                if (!filled)
                {
                    filled = 1;
                    length++;
                    c = Iterate(str); /* Skip over the character */
                    continue;
                }
                /* RFC3513 says the following:
                 * > 'The "::" can only appear once in an address.' */
                *str = start;
                return 0;
            }
            if (digit < 1 || digit > 4)
            {
                goto fail;
            }
            /* We do not have to check whenever the digit here is valid, 
             * because it has to be. */
            digit = 0;
            digits++;

            length++;
            continue;
        }
        /* The only remaining character being '.', we are probably dealing 
         * with an IPv4 literal. */
        *str -= digit + 1;
        length -= digit + 1;
        if (ParseIPv4(str, &ipv4))
        {
            length += strlen(ipv4);
            Free(ipv4);
            c = Iterate(str);
            filled = 1;
            goto end;
        }
    }
end:
    --(*str);
    if (Iterate(str) != ']')
    {
        goto fail;
    }

    length = (size_t) (*str - start);
    if (length < 4 || length > 47)
    {
        goto fail;
    }
    *out = Malloc(length + 1);
    memset(*out, '\0', length + 1);
    memcpy(*out, start, length);

    return 1;
fail:
    *str = start;
    return 0;
}
static int
ParseHostname(char **str, char **out)
{
    char *start;
    size_t length = 0;
    char c;

    start = *str;
    while ((c = Iterate(str)) && 
        (isalnum(c) || c == '.' || c == '-') && 
        ++length < 256)
    {
        /* Do nothing. */
    }
    if (length < 1 || length > 255)
    {
        *str = start;
        return 0;
    }
    length = (size_t) (*str - start) - 1;
    *out = Malloc(length + 1);
    memcpy(*out, start, length);
    (*str)--;
    return 1;
}

static int
ParseServerName(char **str, ServerPart *out)
{
    char c;
    char *start;
    char *startPort;
    size_t chars = 0;

    char *host = NULL;
    char *port = NULL;

    if (!str || !out)
    {
        return 0;
    }

    start = *str;

    if (!host)
    {
        /* If we can parse an IPv4 address, use that. */
        ParseIPv4(str, &host);
    }
    if (!host)
    {
        /* If we can parse an IPv6 address, use that. */
        ParseIPv6(str, &host);
    }
    if (!host)
    {
        /* If we can parse an hostname, use that. */
        ParseHostname(str, &host);
    }
    if (!host)
    {
        /* Can't parse a valid server name. */
        return 0;
    }
    /* Now, there's only 2 options: a ':', or the end(everything else.) */
    if (**str != ':')
    {
        /* We're done. */
        out->hostname = host;
        out->port = NULL;
        return 1;
    }
    /* TODO: Separate this out */
    startPort = ++(*str);
    while(isdigit(c = Iterate(str)) && c && ++chars < 5)
    {
        /* Do nothing. */
    }
    if (chars < 1 || chars > 5)
    {
        *str = start;
        Free(host);
        host = NULL;
        return 0;
    }

    port = Malloc(chars + 1);
    memcpy(port, startPort, chars);
    port[chars] = '\0';
    if (atol(port) > 65535)
    {
        Free(port);
        Free(host);
        *str = start;
        return 0;
    }

    out->hostname = host;
    out->port = port;

    return 1;
}
int
ParseServerPart(char *str, ServerPart *part)
{
    /* This is a wrapper behind the internal ParseServerName. */
    if (!str || !part)
    {
        return 0;
    }
    return ParseServerName(&str, part);
}
void
ServerPartFree(ServerPart part)
{
    if (part.hostname)
    {
        Free(part.hostname);
    }
    if (part.port)
    {
        Free(part.port);
    }
}

int
ParseCommonID(char *str, CommonID *id)
{
    char sigil;

    if (!str || !id)
    {
        return 0;
    }
    
    /* There must at least be 2 chararacters: the sigil and a string.*/
    if (strlen(str) < 2)
    {
        return 0;
    }

    sigil = *str++;
    /* Some sigils have the following restriction:
     * > MUST NOT exceed 255 bytes (including the # sigil and the domain).
     */
    if ((sigil == '#' || sigil == '@') && strlen(str) > 255)
    {
        return 0;
    }
    id->sigil = sigil;
    id->local = NULL;
    id->server.hostname = NULL;
    id->server.port = NULL;
    
    switch (sigil)
    {
        case '$':
            /* For event IDs, it depends on the version, so we're just 
             * accepting it all. */
            if (!ParseUserLocalpart(&str, &id->local))
            {
                return 0;
            }
            if (*str == ':')
            {
                (*str)++;
                if (!ParseServerName(&str, &id->server))
                {
                    Free(id->local);
                    id->local = NULL;
                    return 0;
                }
                return 1;
            }
            break;
        case '!':
        case '#': /* It seems like the localpart should be the same as the 
                     user's: everything, except ':'. */
        case '@':
            if (!ParseUserLocalpart(&str, &id->local))
            {
                return 0;
            }
            if (*str++ != ':')
            {
                Free(id->local);
                id->local = NULL;
                return 0;
            }
            if (!ParseServerName(&str, &id->server))
            {
                Free(id->local);
                id->local = NULL;
                return 0;
            }
            break;
    }
    return 1;
}

void
CommonIDFree(CommonID id)
{
    if (id.local)
    {
        Free(id.local);
    }
    ServerPartFree(id.server);
}
int
ValidCommonID(char *str, char sigil)
{
    CommonID id;
    int ret;
    memset(&id, 0, sizeof(CommonID));
    if (!str)
    {
        return 0;
    }

    ret = ParseCommonID(str, &id) && id.sigil == sigil;
    
    CommonIDFree(id);
    return ret;
}
char *
ParserRecomposeServerPart(ServerPart serverPart)
{
    if (serverPart.hostname && serverPart.port)
    {
        return StrConcat(3, serverPart.hostname, ":", serverPart.port);
    }
    if (serverPart.hostname)
    {
        return StrDuplicate(serverPart.hostname);
    }
    return NULL;
}
char *
ParserRecomposeCommonID(CommonID id)
{
    char *ret = Malloc(2 * sizeof(char));
    ret[0] = id.sigil;
    ret[1] = '\0';

    if (id.local)
    {
        char *tmp = StrConcat(2, ret, id.local);
        Free(ret);

        ret = tmp;
    }
    if (id.server.hostname)
    {
        char *server = ParserRecomposeServerPart(id.server);
        char *tmp = StrConcat(4, "@", ret, ":", server);
        Free(ret);
        Free(server);

        ret = tmp;
    }
    return ret;
}
int
ParserServerNameEquals(ServerPart serverPart, char *str)
{
    char *idServer;
    int ret;
    if (!str)
    {
        return 0;
    }
    idServer = ParserRecomposeServerPart(serverPart);

    ret = StrEquals(idServer, str);
    Free(idServer);
    
    return ret;
}

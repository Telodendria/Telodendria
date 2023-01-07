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
#include <User.h>

#include <string.h>

int
UserValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return 0;
        }

        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              (c == '.') || (c == '_') || (c == '=') || (c == '-') ||
              (c == '/')))
        {
            return 0;
        }

        i++;
    }

    return 1;
}

int
UserHistoricalValidate(char *localpart, char *domain)
{
    size_t maxLen = 255 - strlen(domain) - 1;
    size_t i = 0;

    while (localpart[i])
    {
        char c = localpart[i];

        if (i > maxLen)
        {
            return 0;
        }

        if (!(c >= 0x21 && c <= 0x39) || (c >= 0x3B && c <= 0x7E))
        {
            return 0;
        }

        i++;
    }

    return 1;
}

int
UserExists(Db * db, char *name)
{
    return DbExists(db, 2, "users", name);
}

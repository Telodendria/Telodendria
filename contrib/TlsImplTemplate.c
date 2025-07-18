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
#include <Tls.h>

#if TLS_IMPL == TLS_TEMPLATE       /* Set your TLS_* implementation
                                    * flag here */

/*
 * #include statements and any implementation structures
 * needed should go here.
 */

void *
TlsInitClient(int fd, const char *serverName)
{
    return NULL;
}

void *
TlsInitServer(int fd, const char *crt, const char *key)
{
    return NULL;
}

ssize_t
TlsRead(void *cookie, void *buf, size_t nBytes)
{
    return -1;
}

ssize_t
TlsWrite(void *cookie, void *buf, size_t nBytes)
{
    return -1;
}

int
TlsClose(void *cookie)
{
    return -1;
}

#endif

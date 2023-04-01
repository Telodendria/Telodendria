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
#include <Io.h>

#include <stdio.h>

static ssize_t
IoReadFile(void *cookie, void *buf, size_t nBytes)
{
    FILE *fp = cookie;

    return fread(buf, 1, nBytes, fp);
}

static ssize_t
IoWriteFile(void *cookie, void *buf, size_t nBytes)
{
    FILE *fp = cookie;
    size_t res = fwrite(buf, 1, nBytes, fp);

    /*
     * fwrite() may be buffered on some platforms, but at this low level,
     * it should not be; buffering happens in Stream, not Io.
     */
    fflush(fp);

    return res;
}

static off_t
IoSeekFile(void *cookie, off_t offset, int whence)
{
    FILE *fp = cookie;
    off_t ret = fseeko(fp, offset, whence);

    if (ret > -1)
    {
        return ftello(fp);
    }
    else
    {
        return ret;
    }
}

static int
IoCloseFile(void *cookie)
{
    FILE *fp = cookie;

    return fclose(fp);
}

Io *
IoFile(FILE * fp)
{
    IoFunctions f;

    if (!fp)
    {
        return NULL;
    }

    f.read = IoReadFile;
    f.write = IoWriteFile;
    f.seek = IoSeekFile;
    f.close = IoCloseFile;

    return IoCreate(fp, f);
}

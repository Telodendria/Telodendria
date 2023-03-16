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

    return fwrite(buf, 1, nBytes, fp);
}

static off_t
IoSeekFile(void *cookie, off_t offset, int whence)
{
    FILE *fp = cookie;

    return fseeko(fp, offset, whence);
}

static int
IoCloseFile(void *cookie)
{
    FILE *fp = cookie;

    return fclose(fp);
}

Io *
IoFile(FILE *fp)
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


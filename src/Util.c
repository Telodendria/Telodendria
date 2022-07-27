#include <Util.h>

#include <stdlib.h>
#include <sys/time.h>

long
UtilServerTs(void)
{
    struct timeval tv;
    long ts;

    gettimeofday(&tv, NULL);
    ts = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    return ts;
}

char *
UtilUtf8Encode(unsigned long utf8)
{
    char *str;

    str = malloc(5 * sizeof(char));
    if (!str)
    {
        return NULL;
    }

    if (utf8 <= 0x7F)              /* Plain ASCII */
    {
        str[0] = (char) utf8;
        str[1] = '\0';
    }
    else if (utf8 <= 0x07FF)       /* 2-byte */
    {
        str[0] = (char) (((utf8 >> 6) & 0x1F) | 0xC0);
        str[1] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[2] = '\0';
    }
    else if (utf8 <= 0xFFFF)       /* 3-byte */
    {
        str[0] = (char) (((utf8 >> 12) & 0x0F) | 0xE0);
        str[1] = (char) (((utf8 >> 6) & 0x3F) | 0x80);
        str[2] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[3] = '\0';
    }
    else if (utf8 <= 0x10FFFF)     /* 4-byte */
    {
        str[0] = (char) (((utf8 >> 18) & 0x07) | 0xF0);
        str[1] = (char) (((utf8 >> 12) & 0x3F) | 0x80);
        str[2] = (char) (((utf8 >> 6) & 0x3F) | 0x80);
        str[3] = (char) (((utf8 >> 0) & 0x3F) | 0x80);
        str[4] = '\0';
    }
    else
    {
        /* Send replacement character */
        str[0] = (char) 0xEF;
        str[1] = (char) 0xBF;
        str[2] = (char) 0xBD;
        str[3] = '\0';
    }

    return str;
}

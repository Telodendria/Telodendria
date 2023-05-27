#include <Array.h>
#include <HashMap.h>

#include <Log.h>

int 
Main(Array * args, HashMap * env)
{
    size_t i;
    char *key;
    char *val;

    Log(LOG_INFO, "Hello World!");
    Log(LOG_INFO, "Arguments: %lu", ArraySize(args));

    for (i = 0; i < ArraySize(args); i++)
    {
        Log(LOG_INFO, "  [%ld] %s", i, ArrayGet(args, i));
    }

    Log(LOG_INFO, "Environment:");
    while (HashMapIterate(env, &key, (void **) &val))
    {
        Log(LOG_INFO, "  %s = %s", key, val);
    }

    return 0;
}

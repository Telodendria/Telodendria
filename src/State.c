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

#include <State.h>

#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Array.h>

#include <Room.h>
#include <Event.h>

static HashMap *
StateResolveV1(Array * states)
{
    (void) states;
    return NULL;
}

static HashMap *
StateResolveV2(Array * states)
{
    (void) states;
    return NULL;
}

HashMap *
StateResolve(Room * room, HashMap * event)
{
    Array *states;
    size_t i;

    Array *prevEvents;

    if (!room || !event)
    {
        return NULL;
    }

    /* TODO: Return cached state if it exists */

    states = ArrayCreate();
    if (!states)
    {
        return NULL;
    }

    prevEvents = HashMapGet(event, "prev_events");

    for (i = 0; i < ArraySize(prevEvents); i++)
    {
        HashMap *prevEvent = ArrayGet(prevEvents, i);
        HashMap *state = StateResolve(room, prevEvent);

        /* TODO: Apply prevEvent to state if it is a state event */

        ArrayAdd(states, state);
    }

    switch (RoomVersionGet(room))
    {
        case 1:
            return StateResolveV1(states);
        default:
            return StateResolveV2(states);
    }
}

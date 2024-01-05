/*
 * Copyright (C) 2022-2024 Jordan Bancino <@jordan:bancino.net>
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

#include <Room.h>

#include <Cytoplasm/Memory.h>
#include <Cytoplasm/Str.h>
#include <Cytoplasm/Db.h>

#include <Schema/RoomCreateRequest.h>

struct Room
{
    Db *db;
    DbRef *ref;

    char *id;
    int version;
};

Room *
RoomCreate(Db * db, RoomCreateRequest * req)
{
    (void) db;
    (void) req;
    return NULL;
}

Room *
RoomLock(Db * db, char *id)
{
    DbRef *ref;
    Room *room;

    if (!db || !id)
    {
        return NULL;
    }

    ref = DbLock(db, 3, "rooms", id, "state");

    if (!ref)
    {
        return NULL;
    }

    room = Malloc(sizeof(Room));
    if (!room)
    {
        DbUnlock(db, ref);
        return NULL;
    }

    room->db = db;
    room->ref = ref;
    room->id = StrDuplicate(id);

    return room;
}

int
RoomUnlock(Room * room)
{
    Db *db;
    DbRef *ref;

    if (!room)
    {
        return 0;
    }

    db = room->db;
    ref = room->ref;

    Free(room->id);
    Free(room);

    return DbUnlock(db, ref);
}

char *
RoomIdGet(Room * room)
{
    return room ? room->id : NULL;
}

int
RoomVersionGet(Room * room)
{
    return room ? room->version : 0;
}

HashMap *
RoomStateGet(Room * room)
{
    if (!room)
    {
        return NULL;
    }

    return NULL;
}

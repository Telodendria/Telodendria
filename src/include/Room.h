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
#ifndef TELODENDRIA_ROOM_H
#define TELODENDRIA_ROOM_H

/***
 * @Nm Room
 * @Nd API for creating and manipulating Rooms.
 * @Dd July 6 2023
 * @Xr Event
 *
 * The core component of the Matrix specification is the Room,
 * which holds a directed acyclic graph (DAG) of Events.
 * .Nm
 * provides a way of working with persistent rooms.
 */

#include <Cytoplasm/HashMap.h>
#include <Cytoplasm/Db.h>

#include <Schema/RoomCreateRequest.h>

/**
 * The functions in this API operate on an opaque structure.
 */
typedef struct Room Room;

/**
 * Create a new room in the given database using the given
 * RoomCreateRequest.
 */
extern Room * RoomCreate(Db *, RoomCreateRequest *);

/**
 * Lock the existing room in the specified database,
 * identified by the specified ID, which is expected
 * to include the server name and room ID sigil.
 * .Pp
 * this function returns NULL if there was an error
 * locking the room, such as the room not existing.
 */
extern Room * RoomLock(Db *, char *);

/**
 * Unlock a room handle, returning it to the database.
 * This function returns the result of calling
 * .Fn DbUnlock() on the internal room reference.
 */
extern int RoomUnlock(Room *);

/**
 * Get the full ID of the specified room, including
 * the sigil and server name. This function returns
 * NULL if there was an error getting the room ID.
 */
extern char * RoomIdGet(Room *);

/**
 * Get the numeric room version for the specified
 * room. The room version is determined by and stored
 * in the m.room.create event as a string, but since this
 * is expected to be a relatively frequent operation,
 * the integer representation is stored in the room
 * information so it can be accessed without having
 * to lock the room state and m.room.create event.
 */
extern int RoomVersionGet(Room *);

/**
 * Resolve the state for the latest events in the
 * room. This function uses the appropriate state
 * resolution algorithm to compute the latest state,
 * which is used to select auth events on incoming
 * client events.
 */
extern HashMap * RoomStateGet(Room *);

/**
 * Get a list of the most recent events in the
 * room. When ingressing client events, these events
 * should be copied to the incoming event's prev_events.
 * Note that this function returns an array of actual
 * events themselves, not just IDs, even though that's
 * what's stored in the database.
 */
extern Array * RoomPrevEventsGet(Room *);

/**
 * Set the list of most recent events in the room.
 * When ingressing client events, this list will be
 * manipulated. Unfortunately it can't be manipulated
 * in place with the current design, so this function 
 * is responsible for copying the modified list into
 * place.
 */
extern int RoomPrevEventsSet(Room *, Array *);

/**
 * Send a single event to the specified room. This 
 * function can take either a client event or a
 * federation PDU. Depending on which is supplied,
 * the appropriate logic is applied. Client events
 * are converted into their federation format for 
 * the room version, which includes setting the
 * prev_events and auth_events fields correctly.
 */
extern HashMap * RoomEventSend(Room *, HashMap *);

#endif /* TELODENDRIA_ROOM_H */

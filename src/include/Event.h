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

#ifndef TELODENDRIA_EVENT_H
#define TELODENDRIA_EVENT_H

#include <Cytoplasm/HashMap.h>

#include <Room.h>

/**
 * Compute the content hash of an event. This involves
 * converting it to a canonical JSON string and then
 * hashing it.
 */
extern char * EventContentHash(HashMap *);

/**
 * Get the full event ID, including the sigil, and, if
 * applicable, the server name. This function requires
 * the room handle that the event belongs to because
 * certain room versions have the event ID embedded in
 * the event itself, and others are calculated. Note
 * that if an event is being loaded from the database,
 * the ID is already known. Since this is a computationally
 * and memory intensive operation, it should not be
 * performed more than absolutely necessary, namely, when
 * an event comes in from the federation APIs or the
 * homeserver has finished generating a PDU for a client
 * event.
 */
extern char * EventIdGet(Room *, HashMap *);

/**
 * Redact an event according to the rules for the given
 * room. The event is modified in place.
 */
extern int EventRedact(Room *, HashMap *);

#endif /* TELODENDRIA_EVENT_H */

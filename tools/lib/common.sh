#!/usr/bin/env sh

: "${PATCHES_ROOM:=!tyDHfIvAyfImrzOVVt:bancino.net}"

if [ -f "$(pwd)/.env" ]; then
	. "$(pwd)/.env"
fi

if [ -z "$MXID" ]; then
	MXID="@${USER}:$(hostname)"
fi

if [ -z "$DISPLAY_NAME" ]; then
	DISPLAY_NAME=$(getent passwd "$USER" | cut -d ':' -f 5 | cut -d ',' -f 1)
fi


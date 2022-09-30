#!/usr/bin/env sh

find tools/bin -type f | while IFS= read -r tool; do
	chmod +x "$tool"
done

makewhatis "$(pwd)/man"

export PATH="$(pwd)/tools/bin:$PATH"
export MANPATH="$(pwd)/man:$MANPATH"

PS1="(td) $PS1"

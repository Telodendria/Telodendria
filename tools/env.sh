#!/usr/bin/env sh

find tools/bin -type f | while IFS= read -r tool; do
	chmod +x "$tool"
done

PATH="$(pwd)/tools/bin:$PATH"

#!/usr/bin/env sh

echo "Telodendria Development Environment"
echo "-----------------------------------"
echo
echo "Tools available:"
find tools/bin -type f -not -path '*/CVS/*' | while IFS= read -r tool; do
    echo "- $(basename $tool)"
	chmod +x "$tool"
done

if which makewhatis 2>&1 > /dev/null; then
    makewhatis "$(pwd)/man"
fi

export PATH="$(pwd)/tools/bin:$PATH"
export MANPATH="$(pwd)/man:$MANPATH"


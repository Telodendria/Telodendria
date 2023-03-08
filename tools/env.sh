#!/usr/bin/env sh

echo "Telodendria Development Environment"
echo "-----------------------------------"
echo
echo "Tools available:"
find tools/bin -type f | grep -v CVS | grep -v '#' | while IFS= read -r tool; do
    echo "- $(basename $tool)"
	chmod +x "$tool"
done
find tools/src -type f -name '*.c' | while IFS= read -r tool; do
    echo "- $(basename $tool .c)"
done

if which makewhatis 2>&1 > /dev/null; then
    makewhatis "$(pwd)/man"
fi

export PATH="$(pwd)/tools/bin:$(pwd)/build/tools:$PATH"
export MANPATH="$(pwd)/man:$MANPATH"

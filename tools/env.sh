#!/usr/bin/env sh

echo "Telodendria Development Environment"
echo "-----------------------------------"
echo
echo "Tools available:"
find tools/bin -type f | grep -v CVS | grep -v '#' | while IFS= read -r tool; do
    echo "* $(basename $tool)"
	chmod +x "$tool"
done

missing=0

for tool in $(find tools/src -type f -name '*.c'); do
    base=$(basename "$tool" .c)
    printf "* $base"
    if [ ! -f "build/tools/$base" ]; then
        printf ' (missing)'
        missing=1
    fi
    echo
done

if [ "$missing" -eq "1" ]; then
    echo
    echo "Warning: Some tools are missing, which means others"
    echo "may not work properly. Build missing tools with td."
fi

if which makewhatis 2>&1 > /dev/null; then
    makewhatis "$(pwd)/man"
fi

export PATH="$(pwd)/tools/bin:$(pwd)/build/tools:$PATH"
export MANPATH="$(pwd)/man:$MANPATH"

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

export LD_LIBRARY_PATH="$(pwd)/Cytoplasm/out/lib"
export PATH="$(pwd)/tools/bin:$(pwd)/build/tools:$(pwd)/Cytoplasm/out/bin:$PATH"
export MANPATH="$(pwd)/man:$(pwd)/build/man:$(pwd)/Cytoplasm/out/man:$MANPATH"

if [ "$(uname)" = "OpenBSD" ]; then
    # Other platforms use different MALLOC_OPTIONS
    # flags.
    export MALLOC_OPTIONS="CFGJSU"
fi

export TELODENDRIA_ENV=1


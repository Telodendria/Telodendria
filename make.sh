#!/usr/bin/env sh

TELODENDRIA_VERSION="0.0.1"
CVS_TAG="Telodendria-$(echo $TELODENDRIA_VERSION | sed 's/\./_/g')"

HEADERS="-D_POSIX_C_SOURCE=199506L -DTELODENDRIA_VERSION=\"$TELODENDRIA_VERSION\""
INCLUDES="-Isrc/include"

CC="${CC:-cc}"
CFLAGS="-Wall -Werror -pedantic -std=c89 -O3 $HEADERS $INCLUDES"
LDFLAGS="-static -flto -fdata-sections -ffunction-sections -s -Wl,-static -Wl,-gc-sections"
PROG="telodendria"

mod_time() {
    if [ -n "$1" ] && [ -f "$1" ]; then
        case "$(uname)" in
            Linux)
                stat -c %Y "$1" 
                ;;
            *BSD)
                stat -f %m "$1" 
                ;;
            *)
                echo "0" 
                ;;
        esac 
    else
        echo "0" 
    fi 
}

recipe_build() {
	mkdir -p build

	do_rebuild=0
	objs=""
	for src in $(find src -name '*.c'); do
		obj=$(echo "$src" | sed -e 's/^src/build/' -e 's/\.c$/\.o/')
		objs="$objs $obj"
    
		if [ $(mod_time "$src") -gt $(mod_time "$obj") ]; then
			echo "CC $obj"
			obj_dir=$(dirname "$obj")
			mkdir -p "$obj_dir"
			if ! $CC $CFLAGS -c -o "$obj" "$src"; then
				exit 1
			fi
			do_rebuild=1
		fi
	done

	if [ $do_rebuild -eq 1 ] || [ ! -f "build/$PROG" ]; then
		echo "LD build/$PROG"
		$CC $LDFLAGS -o "build/$PROG" $objs
	else
		echo "Up to date."
	fi
}

recipe_clean() {
	rm -rv build
}

recipe_test() {
	echo "Unit tests are not implemented yet."
}

for recipe in $@; do
	recipe_$recipe
done

if [ -z "$1" ]; then
	recipe_build
fi

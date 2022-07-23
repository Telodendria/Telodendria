#!/usr/bin/env sh

#
# Set variables
#
# This syntax may look odd, but as far as I can tell, it is POSIX,
# and it allows the values to be overridden by the environment,
# such that these serve more as sane defaults than hard requirements.
#

: "${TELODENDRIA_VERSION:=0.0.0}"
: "${CVS_TAG:=Telodendria-$(echo $TELODENDRIA_VERSION | sed 's/\./_/g')}"

: "${HEADERS:=-D_POSIX_C_SOURCE=199506L -DTELODENDRIA_VERSION=\"$TELODENDRIA_VERSION\"}"
: "${INCLUDES:=-Isrc/include}"

: "${CC:=cc}"
: "${CFLAGS:=-Wall -Werror -pedantic -std=c89 -O3 $HEADERS $INCLUDES}"
: "${LDFLAGS:=-static -flto -fdata-sections -ffunction-sections -s -Wl,-static -Wl,-gc-sections}"
: "${PROG:=telodendria}"

if [ -f "$(pwd)/.env" ]; then
	. "$(pwd)/.env"
fi

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

recipe_site() {
	if [ -z "$TELODENDRIA_PUB" ]; then
		echo "No public root directory specified."
		echo "Set TELODENDRIA_PUB."
		exit 1
	fi

	cp "Telodendria.css" "$TELODENDRIA_PUB/"
	cp "Telodendria.html" "$TELODENDRIA_PUB/index.html"
	cp "release/telodendria-signify.pub" "$TELODENDRIA_PUB/"
}

recipe_release() {
	if [ -z "$TELODENDRIA_PUB" ]; then
		echo "No public root directory specified."
		echo "Set TELODENDRIA_PUB."
		exit 1
	fi

	if [ -z "$TELODENDRIA_SIGNIFY_SECRET" ]; then
		echo "No signify secret key specified."
		echo "Set TELODENDRIA_SIGNIFY_SECRET."
		exit 1
	fi

	mkdir -p "$TELODENDRIA_PUB/pub/v$TELODENDRIA_VERSION"
	cd "$TELODENDRIA_PUB/pub/v$TELODENDRIA_VERSION"

	cvs export "-r$CVS_TAG" "Telodendria"
	mv "Telodendria" "Telodendria-v$TELODENDRIA_VERSION"
	tar -czf "Telodendria-v$TELODENDRIA_VERSION.tar.gz" \
		"Telodendria-v$TELODENDRIA_VERSION"
	rm -r "Telodendria-v$TELODENDRIA_VERSION"
	sha256 "Telodendria-v$TELODENDRIA_VERSION.tar.gz" \
		> "Telodendria-v$TELODENDRIA_VERSION.tar.gz.sha256"
	signify -S -s "$TELODENDRIA_SIGNIFY_SECRET" \
		-m "Telodendria-v$TELODENDRIA_VERSION.tar.gz" \
		-x "Telodendria-v$TELODENDRIA_VERSION.tar.gz.sig"
}

for recipe in $@; do
	recipe_$recipe
done

if [ -z "$1" ]; then
	recipe_build
fi


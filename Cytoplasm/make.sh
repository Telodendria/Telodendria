#!/usr/bin/env sh

addprefix() {
	prefix="$1"
	shift
	for thing in "$@"; do
		echo "${prefix}${thing}"
	done

	unset prefix
	unset thing
}

: "${NAME:=Cytoplasm}"
: "${LIB_NAME:=$(echo ${NAME} | tr '[A-Z]' '[a-z]')}"
: "${VERSION:=0.3.0}"

: "${CVS_TAG:=${NAME}-$(echo ${VERSION} | sed 's/\./_/g')}"


: "${SRC:=src}"
: "${TOOLS:=tools}"
: "${BUILD:=build}"
: "${OUT:=out}"
: "${STUB:=RtStub}"
: "${LICENSE:=LICENSE.txt}"

: "${CC:=cc}"
: "${AR:=ar}"

: "${DEFINES:=-D_DEFAULT_SOURCE -DLIB_NAME=\"${NAME}\" -DLIB_VERSION=\"${VERSION}\"}"
: "${INCLUDE:=${SRC}/include}"

: "${CFLAGS:=-Wall -Wextra -pedantic -std=c89 -O3 -pipe}"
: "${LD_EXTRA:=-flto -fdata-sections -ffunction-sections -s -Wl,-gc-sections}"
: "${LDFLAGS:=-lm -pthread}"

if [ "${DEBUG}" = "1" ]; then
    CFLAGS="${CFLAGS} -O0 -g"
    LD_EXTRA=""
fi

if [ -n "${TLS_IMPL}" ]; then
    case "${TLS_IMPL}" in
        "LIBRESSL")
            TLS_LIBS="-ltls -lcrypto -lssl"
            ;;
        "OPENSSL")
            TLS_LIBS="-lcrypto -lssl"
            ;;
        *)
            echo "Unrecognized TLS implementation: ${TLS_IMPL}"
            echo "Consult Tls.h for supported implementations."
            echo "Note that the TLS_ prefix is omitted in TLS_IMPL."
            exit 1
            ;;
    esac

    DEFINES="${DEFINES} -DTLS_IMPL=TLS_${TLS_IMPL}"
    LDFLAGS="${LDFLAGS} ${TLS_LIBS}"
fi

CFLAGS="${CFLAGS} ${DEFINES} $(addprefix -I$(pwd)/ ${INCLUDE})"
LDFLAGS="${LDFLAGS} ${LD_EXTRA}"

# Check the modificiation time of a file. This is used to do
# incremental builds; we only want to rebuild files that have
# have changed.
mod_time() {
    if [ -n "$1" ] && [ -f "$1" ]; then
        case "$(uname)" in
            Linux | CYGWIN_NT* | Haiku)
                stat -c %Y "$1"
                ;;
            *BSD | DragonFly | Minix)
                stat -f %m "$1"
                ;;
            *)
                # Platform unknown, force rebuilding the whole
                # project every time.
                echo "0"
                ;;
        esac
    else
        echo "0"
    fi
}

# Substitute shell variables in a stream with their actual value
# in this shell.
setsubst() {
    SED="/tmp/sed-$RANDOM.txt"

    (
        set | while IFS='=' read -r var val; do
            val=$(echo "$val" | cut -d "'" -f 2- | rev | cut -d "'" -f 2- | rev)
            echo "s|\\\${$var}|$val|g"
        done

        echo "s|\\\${[a-zA-Z_]*}||g"
        echo "s|'''|'|g"
    ) >"$SED"

    sed -f "$SED" $@
    rm "$SED"
}

recipe_docs() {
	export LD_LIBRARY_PATH="${OUT}/lib"
    mkdir -p "${OUT}/man/man3"

    for header in $(find ${INCLUDE} -name '*.h'); do
        basename=$(basename "$header")
        man=$(echo "${OUT}/man/man3/$basename" | sed -e 's/\.h$/\.3/')

        if [ $(mod_time "$header") -ge $(mod_time "$man") ]; then
            echo "DOC $basename"
            if ! "${OUT}/bin/hdoc" -D "Os=${NAME}" -i "$header" -o "$man"; then
                rm "$man"
                exit 1
            fi
        fi
    done

    if which makewhatis 2>&1 > /dev/null; then
        makewhatis "${OUT}/man"
    fi
}

recipe_libs() {
	echo "-lm -pthread ${TLS_LIBS}"
}

recipe_build() {
    mkdir -p "${BUILD}" "${OUT}/bin" "${OUT}/lib"
    cd "${SRC}"


    echo "CC = ${CC}"
    echo "CFLAGS = ${CFLAGS}"
    echo "LDFLAGS = ${LDFLAGS}"
    echo

    do_rebuild=0
    objs=""
    for src in $(find . -name '*.c' | cut -d '/' -f 2-); do
        obj=$(echo "${BUILD}/$src" | sed -e 's/\.c$/\.o/')

        if [ $(basename "$obj" .o) != "${STUB}" ]; then
            objs="$objs $obj"
        fi

        if [ $(mod_time "$src") -ge $(mod_time "../$obj") ]; then
            echo "CC $(basename $obj)"
            obj_dir=$(dirname "../$obj")
            mkdir -p "$obj_dir"
            if ! $CC $CFLAGS -fPIC -c -o "../$obj" "$src"; then
                exit 1
            fi
            do_rebuild=1
        fi
    done

    cd ..

    if [ $do_rebuild -eq 1 ] || [ ! -f "${OUT}/lib/lib${LIB_NAME}.a" ]; then
        echo "AR lib${LIB_NAME}.a"
        if ! $AR rcs "${OUT}/lib/lib${LIB_NAME}.a" $objs; then
            exit 1
        fi
    fi

    if [ $do_rebuild -eq 1 ] || [ ! -f "${OUT}/lib/lib${LIB_NAME}.so" ]; then
        echo "LD lib${LIB_NAME}.so"
        if ! $CC -shared -o "${OUT}/lib/lib${LIB_NAME}.so" $objs ${LDFLAGS}; then
            exit 1
        fi
    fi

    cp "${BUILD}/${STUB}.o" "${OUT}/lib/${LIB_NAME}.o"

    for src in $(find "${TOOLS}" -name '*.c'); do
        out=$(basename "$src" .c)
        out="${OUT}/bin/$out"

        if [ $(mod_time "$src") -ge $(mod_time "$out") ] || [ $do_rebuild -eq 1 ]; then
            echo "CC $(basename $out)"
            mkdir -p "$(dirname $out)"
            if ! $CC $CFLAGS -o "$out" "$src" "${OUT}/lib/${LIB_NAME}.o" "-L${OUT}/lib" "-l${LIB_NAME}" ${LDFLAGS}; then
                exit 1
            fi
        fi
    done

    recipe_docs
}

recipe_clean() {
    rm -r "${BUILD}" "${OUT}"
}

# Update copyright comments in sources and header files.
recipe_license() {
    find . -name '*.[ch]' | while IFS= read -r src; do
        if [ -t 1 ]; then
            printf "LICENSE %s%*c\r" $(basename "$src") "16" " "
        fi
        srcHeader=$(grep -n -m 1 '^ \*/' "$src" | cut -d ':' -f 1)
        head "-n$srcHeader" "$src" |
            diff -u -p - "${LICENSE}" |
            patch "$src" | grep -v "^Hmm"
    done
    if [ -t 1 ]; then
        printf "%*c\n" "50" " "
    fi
}

# Format source code files by running indent(1) on them.
recipe_format() {
    find . -name '*.c' | while IFS= read -r src; do
        if [ -t 1 ]; then
            printf "FMT %s%*c\r" $(basename "$src") "16" " "
        fi
        if indent "$src"; then
            rm $(basename "$src").BAK
        fi
    done
    if [ -t 1 ]; then
        printf "%*c\n" "50" " "
    fi
}

# Generate a release tarball, checksum and sign it, and push it to
# the web root.
recipe_release() {
    # Tag the release at this point in time.
    cvs tag "$CVS_TAG"

	mkdir -p "${OUT}/release"
    cd "${OUT}/release"

    # Generate the release tarball.
    cvs export "-r$CVS_TAG" "${NAME}"
    mv "${NAME}" "${NAME}-v${VERSION}"
    tar -czf "${NAME}-v${VERSION}.tar.gz" "${NAME}-v${VERSION}"
    rm -r "${NAME}-v${VERSION}"

    # Checksum the release tarball.
    sha256 "${NAME}-v${VERSION}.tar.gz" > "${NAME}-v${VERSION}.tar.gz.sha256"

    # Sign the release tarball.
    if [ ! -z "${SIGNIFY_SECRET}" ]; then
        signify -S -s "${SIGNIFY_SECRET}" \
            -m "${NAME}-v${VERSION}.tar.gz" \
            -x "${NAME}-v${VERSION}.tar.gz.sig"
    else
        echo "Warning: SIGNIFY_SECRET not net."
        echo "The built tarball will not be signed."
    fi
}

recipe_patch() {
    # If the user has not set their MXID, try to deduce one from
    # their system.
    if [ -z "$MXID" ]; then
        MXID="@${USER}:$(hostname)"
    fi

    # If the user has not set their EDITOR, use a safe default.
    # (vi should be available on any POSIX system)
    if [ -z "$EDITOR" ]; then
        EDITOR=vi
    fi

    NORMALIZED_MXID=$(echo "$MXID" | sed -e 's/@//g' -e 's/:/-/g')
    PATCH_FILE="${NORMALIZED_MXID}_$(date +%s).patch"

    # Generate the actual patch file
    # Here we write some nice headers, and then do a cvs diff.
    (
        printf 'From: "%s" <%s>\n' "$DISPLAY_NAME" "$MXID"
        echo "Date: $(date)"
        echo "Subject: "
        echo
        cvs -q diff -uNp $PATCHSET | grep -v '^\? '
    ) >"$PATCH_FILE"

    "$EDITOR" "$PATCH_FILE"
    echo "$PATCH_FILE"
}

recipe_diff() {
    tmp_patch="/tmp/${NAME}-$(date +%s).patch"
    cvs -q diff -uNp $PATCHSET >"$tmp_patch"
    if [ -z "$PAGER" ]; then
        PAGER="less -F"
    fi

    $PAGER "$tmp_patch"
    rm "$tmp_patch"
}

# Execute the user-specified recipes.
for recipe in $@; do
    recipe_$recipe
done

# If no recipe was provided, run a build.
if [ -z "$1" ]; then
    recipe_build
fi

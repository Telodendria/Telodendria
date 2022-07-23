#!/usr/bin/env sh

if [ -z "$TELODENDRIA_PUB" ]; then
	echo "No public root directory specified."
	echo "Set TELODENDRIA_PUB."
	exit 1
fi

cp "Telodendria.css" "$TELODENDRIA_PUB/"
cp "Telodendria.html" "$TELODENDRIA_PUB/index.html"
cp "release/telodendria-signify.pub" "$TELODENDRIA_PUB/"

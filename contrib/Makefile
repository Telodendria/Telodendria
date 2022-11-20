all:
	sh tools/bin/td

install:
	install build/telodendria $(PREFIX)/bin/telodendria
	mkdir -p $(PREFIX)/share/examples/telodendria
	install contrib/production.conf $(PREFIX)/share/examples/telodendria/telodendria.conf
	find man -name 'telodendria*\.[1-8]' -exec install {} $(PREFIX)/{} \;

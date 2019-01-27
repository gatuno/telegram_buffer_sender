SUBDIRS = po src data
#data etc

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = build-aux/config.rpath

.PHONY: install_systemd

install_systemd:
	cd data && $(MAKE) install_systemd

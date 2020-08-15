configdir = $(sysconfdir)/telegrambf
config_DATA = telegrambf.conf

EXTRA_DIST = telegrambf@.service.in telegrambf.in telegrambf.conf.in
CLEANFILES = telegrambf.conf telegrambf@.service telegrambf

.PHONY: install_debian_sysvinit install_systemd

install_debian_sysvinit: telegrambf
	$(INSTALL_SCRIPT) telegrambf /etc/init.d/telegrambf
	update-rc.d telegrambf defaults

install_systemd: telegrambf@.service
	$(INSTALL_SCRIPT) telegrambf@.service /etc/systemd/system/
#	systemctl daemon-reload
#	systemctl enable telegrambf.service
#	systemctl start telegrambf.service

#install_logrotate: tftp.logrotate
#	$(INSTALL) -m 0644 tftp.logrotate $(sysconfdir)/logrotate.d/yoshi_tftp

do_subst = sed -e 's|@pkglibdir[@]|$(pkglibdir)|g' \
	       -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	       -e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	       -e 's|@localstatedir[@]|$(localstatedir)|g' \
	       -e 's|@runstatedir[@]|$(runstatedir)|g'

telegrambf.conf: telegrambf.conf.in
	$(AM_V_GEN) $(do_subst) $< > $@

telegrambf: telegrambf.in
	$(AM_V_GEN) $(do_subst) $< > $@

telegrambf@.service: telegrambf@.service.in
	$(AM_V_GEN) $(do_subst) $< > $@

#tftp.logrotate: tftp.logrotate.in
#	$(AM_V_GEN) $(do_subst) $< > $@


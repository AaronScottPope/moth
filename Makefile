BASE = /opt/ctf
VAR = $(BASE)/var
WWW = $(BASE)/www
LIB = $(BASE)/lib
BIN = $(BASE)/bin
SBIN = $(BASE)/sbin
BASE_URL = /ctf/

TEMPLATE = $(CURDIR)/template.html
MDWNTOHTML = $(CURDIR)/mdwntohtml.py --template=$(TEMPLATE) --base=$(BASE_URL)

default: install

SUBDIRS = mdwn
INSTALL_TARGETS = $(addsuffix -install, $(SUBDIRS))
include $(addsuffix /*.mk, $(SUBDIRS))

install: base-install $(INSTALL_TARGETS)
	install --directory --owner=ctf $(VAR)/tanks
	install --directory --owner=ctf $(VAR)/tanks/results
	install --directory --owner=ctf $(VAR)/tanks/errors
	install --directory --owner=ctf $(VAR)/tanks/ai
	install --directory --owner=ctf $(VAR)/tanks/ai/players
	install --directory --owner=ctf $(VAR)/tanks/ai/house

	echo 'VAR = "$(VAR)"' > ctf/paths.py
	echo 'WWW = "$(WWW)"' >> ctf/paths.py
	echo 'LIB = "$(LIB)"' >> ctf/paths.py
	echo 'BIN = "$(BIN)"' >> ctf/paths.py
	echo 'SBIN = "$(SBIN)"' >> ctf/paths.py
	echo 'BASE_URL = "$(BASE_URL)"' >> ctf/paths.py
	python setup.py install

	install bin/pointscli $(BIN)
	install bin/in.pointsd bin/in.flagd \
		bin/scoreboard bin/run-tanks \
		bin/run-ctf $(SBIN)
	cp -r lib/* $(LIB)
	cp -r www/* $(WWW)
	rm -f $(WWW)/tanks/results
	ln -s $(VAR)/tanks/results $(WWW)/tanks/results
	cp template.html $(LIB)

	./mkpuzzles.py --base=$(BASE_URL) --puzzles=puzzles \
		--htmldir=$(WWW)/puzzler --keyfile=$(LIB)/puzzler.keys

	install --directory $(VAR)/disabled
	touch $(VAR)/disabled/bletchley
	touch $(VAR)/disabled/compaq
	touch $(VAR)/disabled/crypto
	touch $(VAR)/disabled/forensics
	touch $(VAR)/disabled/hackme
	touch $(VAR)/disabled/hispaniola
	touch $(VAR)/disabled/net-re
	touch $(VAR)/disabled/skynet
	touch $(VAR)/disabled/survey


base-install:
	id ctf || useradd --system --shell /bin/false --home $(VAR) \
		--comment "Capture The Flag" ctf
	install --directory $(LIB) $(BIN) $(SBIN)
	install --directory --owner=ctf $(VAR)
	install --directory --owner=ctf $(WWW)
	install --directory --owner=ctf $(WWW)/puzzler
	install --directory --owner=ctf $(VAR)/points
	install --directory --owner=ctf $(VAR)/points/tmp
	install --directory --owner=ctf $(VAR)/points/cur
	install --directory --owner=ctf $(VAR)/flags


uninstall:
	rm -rf $(VAR) $(WWW) $(LIB) $(BIN) $(SBIN)
	rmdir $(BASE) || true


clean: $(addsuffix -clean, $(SUBDIRS))

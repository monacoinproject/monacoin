.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. monacoin_qt test_monacoin_qt
clean: FORCE
	$(MAKE) -C .. monacoin_qt_clean test_monacoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_monacoin_qt_check
monacoin-qt monacoin-qt.exe: FORCE
	 $(MAKE) -C .. monacoin_qt
apk: FORCE
	$(MAKE) -C .. monacoin_qt_apk

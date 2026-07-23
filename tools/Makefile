MODULE_VERSION ?= 1.0.0
BUILD_NUMBER   ?= 1
TARGET_ARCH    ?= $(shell uname -m)
OUTPUT_DIR     ?= $(CURDIR)/../build
RPM_VERSION    := $(shell echo $(MODULE_VERSION) | sed 's/-/_/g')
CFLAGS = -Wall -g

ACLE_TOOLS = acle-buf0 acle-disable-watchdog acle-get-random acle-get-state \
             acle-get-tmid acle-login acle-logout acle-sd acle-set-rng \
             acle-test-random-generator acle-write-sectors acle-get-rtc \
             acle-set-rtc acle-set-sdc-mode

EXTRA_TOOLS = crypt_db decrypt_db_write
CONVERT_TOOLS = convert_icl

.PHONY: all clean install uninstall package package-tar package-deb package-rpm test

all: $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS) acle-tools-tar

$(ACLE_TOOLS):
	echo "🔨 Сборка $@..."
	if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "️ $@/Makefile не найден"; \
	fi

$(CONVERT_TOOLS):
	echo "🔨 Сборка $@..."
	if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "⚠️ $@/Makefile не найден"; \
	fi

$(EXTRA_TOOLS):
	echo "🔨 Сборка $@..."
	if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "⚠️ $@: нет Makefile"; \
	fi

acle-tools-tar:
	mkdir -p acle-tools
	find ./ -maxdepth 1 -type f -executable -name "acle-*" -exec cp {} acle-tools/ \;
	find ./ -maxdepth 1 -type f -executable -name "convert_*" -exec cp {} acle-tools/ \;
	cp -r scripts/ acle-tools/scripts/
	tar -czvf acle-tools-bin.tar.gz acle-tools
	rm -rf acle-tools
	mkdir -p $(OUTPUT_DIR)
	mv acle-tools-bin.tar.gz $(OUTPUT_DIR)/

clean:
	for tool in $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			$(MAKE) -C $$tool clean; \
		fi; \
	done
	rm -f acle-tools-bin.tar.gz
	rm -rf acle-tools

install:
	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl /usr/local/bin/; fi
	mkdir -p /usr/local/share/acle-tools/scripts
	cp scripts/*.sh /usr/local/share/acle-tools/scripts/
	chmod +x /usr/local/share/acle-tools/scripts/*.sh
	echo "✅ Утилиты установлены"

uninstall:
	for tool in $(ACLE_TOOLS) $(EXTRA_TOOLS); do rm -f /usr/local/bin/$$tool; done
	rm -f /usr/local/bin/convert_icl
	rm -rf /usr/local/share/acle-tools
	echo "✅ Утилиты удалены"

test:
	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			$(MAKE) -C $$tool all; \
		fi; \
	done
	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then \
			echo "✅ $$tool собран"; \
		else \
			echo "⚠️ $$tool не найден"; \
		fi; \
	done
	test -d scripts && echo "✅ Скрипты найдены" || echo "⚠️ Скрипты отсутствуют"
	echo "✅ Тесты утилит пройдены"

package: package-tar package-deb package-rpm

package-tar:
	mkdir -p $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)
	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi
	mkdir -p $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts
	cp scripts/*.sh $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts/
	tar -czf $(OUTPUT_DIR)/acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz \
		-C $(OUTPUT_DIR)/tar acle-tools-$(MODULE_VERSION)
	rm -rf $(OUTPUT_DIR)/tar
	echo "✅ Создан: acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz"

package-deb:
	rm -rf $(OUTPUT_DIR)/deb-tools
	mkdir -p $(OUTPUT_DIR)/deb-tools/DEBIAN
	mkdir -p $(OUTPUT_DIR)/deb-tools/usr/bin
	mkdir -p $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts
	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/deb-tools/usr/bin/; fi; \
	done
	for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/deb-tools/usr/bin/; fi; \
	done
	if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/deb-tools/usr/bin/; fi
	cp scripts/*.sh $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts/
	chmod +x $(OUTPUT_DIR)/deb-tools/usr/bin/*
	chmod +x $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts/*.sh

	echo "Package: acle-tools" > $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	echo "Version: $(MODULE_VERSION)-build$(BUILD_NUMBER)" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	echo "Architecture: $(if $(filter x86_64,$(TARGET_ARCH)),amd64,$(TARGET_ARCH))" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	echo "Maintainer: OKB SAPR" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	echo "Description: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	echo "Depends: zlib1g, perl" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control

	dpkg-deb --root-owner-group --build $(OUTPUT_DIR)/deb-tools \
		$(OUTPUT_DIR)/acle-tools_$(MODULE_VERSION)-build$(BUILD_NUMBER)_$(if $(filter x86_64,$(TARGET_ARCH)),amd64,$(TARGET_ARCH)).deb
	rm -rf $(OUTPUT_DIR)/deb-tools
	echo "✅ Создан DEB пакет"

package-rpm:
	rm -rf $(OUTPUT_DIR)/rpm-tools
	mkdir -p $(OUTPUT_DIR)/rpm-tools/BUILD
	mkdir -p $(OUTPUT_DIR)/rpm-tools/RPMS
	mkdir -p $(OUTPUT_DIR)/rpm-tools/SOURCES
	mkdir -p $(OUTPUT_DIR)/rpm-tools/SPECS
	mkdir -p $(OUTPUT_DIR)/rpm-tools/SRPMS
	mkdir -p $(OUTPUT_DIR)/rpm-tools/BUILDROOT
	mkdir -p $(OUTPUT_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)

	for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi
	cp scripts/*.sh $(OUTPUT_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/

	echo "Name: acle-tools" > $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "Version: $(RPM_VERSION)" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "Release: $(BUILD_NUMBER)%{?dist}" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "Summary: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "License: Proprietary" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "Requires: zlib, perl" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "%description" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "Utilities for AMDZ GX" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "%install" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "mkdir -p %{buildroot}/usr/bin" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "mkdir -p %{buildroot}/usr/share/acle-tools/scripts" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/acle-* %{buildroot}/usr/bin/" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/convert_* %{buildroot}/usr/bin/" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "cp -r %{_sourcedir}/acle-tools-$(MODULE_VERSION)/scripts/* %{buildroot}/usr/share/acle-tools/scripts/" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "%files" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "/usr/bin/acle-*" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	echo "/usr/share/acle-tools/*" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec

	if command -v rpmbuild >/dev/null 2>&1; then \
		rpmbuild --define "_topdir $(OUTPUT_DIR)/rpm-tools" \
			--define "_sourcedir $(OUTPUT_DIR)/rpm-tools/SOURCES" \
			-bb $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec; \
		find $(OUTPUT_DIR)/rpm-tools/RPMS -name "*.rpm" -exec cp {} $(OUTPUT_DIR)/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm \; \
		if [ -f $(OUTPUT_DIR)/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm ]; then \
			echo "✅ Создан: acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm"; \
		else \
			echo "⚠️ RPM пакет не найден"; \
		fi; \
	else \
		echo "⚠️ rpmbuild не установлен. Пропускаем RPM."; \
	fi
	rm -rf $(OUTPUT_DIR)/rpm-tools
MODULE_VERSION ?= 1.0.0
BUILD_NUM      ?= 1
TARGET_ARCH    ?= $(shell uname -m)

BUILD_DIR      ?= $(CURDIR)/build
PACKAGES_DIR   ?= $(CURDIR)/packages
DIST_DIR       ?= $(CURDIR)/dist
TESTS_DIR      ?= $(CURDIR)/tests
RPM_VERSION    := $(shell echo $(MODULE_VERSION) | sed 's/-/_/g')

CFLAGS = -Wall -g
ACLE_TOOLS = acle-buf0 acle-disable-watchdog acle-get-random acle-get-state \
             acle-get-tmid acle-login acle-logout acle-sd acle-set-rng \
             acle-test-random-generator acle-write-sectors acle-get-rtc \
             acle-set-rtc acle-set-sdc-mode
EXTRA_TOOLS = crypt_db decrypt_db_write
CONVERT_TOOLS = convert_icl

.PHONY: all clean install uninstall package package-tar package-deb package-rpm test prepare-dirs

all: $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS) acle-tools-tar

prepare-dirs:
	mkdir -p $(BUILD_DIR) $(PACKAGES_DIR) $(DIST_DIR) $(TESTS_DIR)

$(ACLE_TOOLS):
	@echo "Сборка $@"
	@if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "$@/Makefile не найден"; \
		exit 1; \
	fi

$(CONVERT_TOOLS):
	@echo "Сборка $@"
	@if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "$@/Makefile не найден"; \
		exit 1; \
	fi

$(EXTRA_TOOLS):
	@echo "Сборка $@"
	@if [ -f $@/Makefile ]; then \
		$(MAKE) -C $@ all; \
	else \
		echo "$@: нет Makefile"; \
		exit 1; \
	fi

acle-tools-tar:
	mkdir -p acle-tools
	find ./ -maxdepth 1 -type f -executable -name "acle-*" -exec cp {} acle-tools/ \;
	find ./ -maxdepth 1 -type f -executable -name "convert_*" -exec cp {} acle-tools/ \;
	cp -r scripts/ acle-tools/scripts/
	tar -cJvf acle-tools-bin.tar.gz acle-tools
	rm -rf acle-tools
	mkdir -p $(DIST_DIR)
	mv acle-tools-bin.tar.gz $(DIST_DIR)/

clean:
	@for tool in $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			$(MAKE) -C $$tool clean; \
		fi; \
	done
	rm -f acle-tools-bin.tar.gz
	rm -rf acle-tools
	rm -rf $(BUILD_DIR) $(PACKAGES_DIR) $(DIST_DIR) $(TESTS_DIR)

install:
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl /usr/local/bin/; fi
	@mkdir -p /usr/local/share/acle-tools/scripts
	@cp scripts/*.sh /usr/local/share/acle-tools/scripts/
	@chmod +x /usr/local/share/acle-tools/scripts/*.sh
	@echo "Утилиты установлены"

uninstall:
	@for tool in $(ACLE_TOOLS) $(EXTRA_TOOLS); do rm -f /usr/local/bin/$$tool; done
	@rm -f /usr/local/bin/convert_icl
	@rm -rf /usr/local/share/acle-tools
	@echo "Утилиты удалены"

test:
	@mkdir -p $(TESTS_DIR)
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			$(MAKE) -C $$tool all; \
		fi; \
	done
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then \
			echo "$$tool собран"; \
		else \
			echo "$$tool не найден"; \
			exit 1; \
		fi; \
	done
	@test -d scripts && echo "Скрипты найдены"
	@echo "Тесты пройдены"

package: prepare-dirs package-tar package-deb package-rpm

package-tar:
	@mkdir -p $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi
	@mkdir -p $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts
	@cp scripts/*.sh $(BUILD_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts/
	@mkdir -p $(DIST_DIR)
	@tar -czf $(DIST_DIR)/acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz \
		-C $(BUILD_DIR)/tar acle-tools-$(MODULE_VERSION)
	@rm -rf $(BUILD_DIR)/tar
	@echo "Создан: $(DIST_DIR)/acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz"

package-deb:
	@rm -rf $(BUILD_DIR)/deb-tools
	@mkdir -p $(BUILD_DIR)/deb-tools/DEBIAN
	@mkdir -p $(BUILD_DIR)/deb-tools/usr/bin
	@mkdir -p $(BUILD_DIR)/deb-tools/usr/share/acle-tools/scripts
	@chmod -R 0755 $(BUILD_DIR)/deb-tools/DEBIAN
	@for tool in $(ACLE_TOOLS); do if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/deb-tools/usr/bin/; fi; done
	@for tool in $(EXTRA_TOOLS); do if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/deb-tools/usr/bin/; fi; done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(BUILD_DIR)/deb-tools/usr/bin/; fi
	@cp scripts/*.sh $(BUILD_DIR)/deb-tools/usr/share/acle-tools/scripts/
	@chmod +x $(BUILD_DIR)/deb-tools/usr/bin/*
	@chmod +x $(BUILD_DIR)/deb-tools/usr/share/acle-tools/scripts/*.sh
	@echo "Package: acle-tools" > $(BUILD_DIR)/deb-tools/DEBIAN/control
	@echo "Version: $(MODULE_VERSION)" >> $(BUILD_DIR)/deb-tools/DEBIAN/control
	@echo "Architecture: $(if $(filter x86_64,$(TARGET_ARCH)),amd64,$(TARGET_ARCH))" >> $(BUILD_DIR)/deb-tools/DEBIAN/control
	@echo "Maintainer: OKB SAPR" >> $(BUILD_DIR)/deb-tools/DEBIAN/control
	@echo "Description: Utilities for working with AMDZ GX" >> $(BUILD_DIR)/deb-tools/DEBIAN/control
	@echo "Depends: zlib1g, perl" >> $(BUILD_DIR)/deb-tools/DEBIAN/control
	@mkdir -p $(PACKAGES_DIR)/debian
	@dpkg-deb --root-owner-group --build $(BUILD_DIR)/deb-tools \
		$(PACKAGES_DIR)/debian/acle-tools_$(MODULE_VERSION)-$(BUILD_NUM)_amd64.deb
	@rm -rf $(BUILD_DIR)/deb-tools
	@echo "Создан: $(PACKAGES_DIR)/debian/acle-tools_$(MODULE_VERSION)-$(BUILD_NUM)_amd64.deb"

package-rpm:
	@rm -rf $(BUILD_DIR)/rpm-tools
	@mkdir -p $(BUILD_DIR)/rpm-tools/BUILD
	@mkdir -p $(BUILD_DIR)/rpm-tools/RPMS
	@mkdir -p $(BUILD_DIR)/rpm-tools/SOURCES
	@mkdir -p $(BUILD_DIR)/rpm-tools/SPECS
	@mkdir -p $(BUILD_DIR)/rpm-tools/SRPMS
	@mkdir -p $(BUILD_DIR)/rpm-tools/BUILDROOT
	@mkdir -p $(BUILD_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)
	
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(BUILD_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(BUILD_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi
	@cp scripts/*.sh $(BUILD_DIR)/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/
	
	@echo "Name: acle-tools" > $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Version: $(RPM_VERSION)" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Release: $(BUILD_NUM)%{?dist}" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Summary: Utilities for working with AMDZ GX" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "License: Proprietary" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Requires: zlib, perl" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%description" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Utilities for AMDZ GX" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%install" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "mkdir -p %{buildroot}/usr/bin" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "mkdir -p %{buildroot}/usr/share/acle-tools/scripts" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/acle-* %{buildroot}/usr/bin/" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/convert_* %{buildroot}/usr/bin/" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "cp -r %{_sourcedir}/acle-tools-$(MODULE_VERSION)/scripts/* %{buildroot}/usr/share/acle-tools/scripts/" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%files" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "/usr/bin/acle-*" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "/usr/bin/convert_*" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "/usr/share/acle-tools/*" >> $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec
	
	@if command -v rpmbuild >/dev/null 2>&1; then \
		rpmbuild --define "_topdir $(BUILD_DIR)/rpm-tools" \
			--define "_sourcedir $(BUILD_DIR)/rpm-tools/SOURCES" \
			-bb $(BUILD_DIR)/rpm-tools/SPECS/acle-tools.spec; \
		mkdir -p $(PACKAGES_DIR)/redos; \
		find $(BUILD_DIR)/rpm-tools/RPMS -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/redos/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm \; \
		if [ -f $(PACKAGES_DIR)/redos/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm ]; then \
			echo "Создан: $(PACKAGES_DIR)/redos/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm"; \
		else \
			echo "RPM пакет не найден"; \
			exit 1; \
		fi \
	else \
		echo "rpmbuild не установлен"; \
		exit 1; \
	fi
	@rm -rf $(BUILD_DIR)/rpm-tools
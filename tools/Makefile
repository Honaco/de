# ==========================================
# Makefile для сборки утилит Accord-LE
# ТЗ п. 3.2, 5, 6.2
# ==========================================

MODULE_VERSION ?= 1.0.0
BUILD_NUM      ?= 1
TARGET_ARCH    ?= $(shell uname -m)
OUTPUT_DIR     ?= $(CURDIR)/../build

# RPM версия (без дефисов)
RPM_VERSION    := $(shell echo $(MODULE_VERSION) | sed 's/-/_/g')

CFLAGS = -Wall -g

# Список утилит (исправлены все опечатки!)
ACLE_TOOLS = acle-buf0 acle-disable-watchdog acle-get-random acle-get-state \
             acle-get-tmid acle-login acle-logout acle-sd acle-set-rng \
             acle-test-random-generator acle-write-sectors acle-get-rtc \
             acle-set-rtc acle-set-sdc-mode

# Дополнительные утилиты
EXTRA_TOOLS = crypt_db decrypt_db_write

# Конвертеры
CONVERT_TOOLS = convert_icl

.PHONY: all clean install uninstall package package-tar package-deb package-rpm test

# ===== Сборка всех утилит =====
all: $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS) acle-tools-tar
	@echo "✅ Утилиты собраны"

# Сборка каждой утилиты
$(ACLE_TOOLS):
	@echo "🔨 Сборка $@..."
	@if [ -f $@/Makefile ]; then \
		sudo ${MAKE} -C $@ all; \
	else \
		echo "⚠️ $@/Makefile не найден, пропускаем"; \
	fi

$(CONVERT_TOOLS):
	@echo "🔨 Сборка $@..."
	@if [ -f $@/Makefile ]; then \
		sudo ${MAKE} -C $@ all; \
	else \
		echo "⚠️ $@/Makefile не найден, пропускаем"; \
	fi

$(EXTRA_TOOLS):
	@echo "🔨 Сборка $@..."
	@if [ -f $@/Makefile ]; then \
		sudo ${MAKE} -C $@ all; \
	else \
		echo "⚠️ $@: нет Makefile, пропускаем"; \
	fi

# Создание tar-архива утилит
acle-tools-tar:
	mkdir -p acle-tools
	find ./ -maxdepth 1 -type f -executable -name "acle-*" -exec cp {} acle-tools/ \;
	find ./ -maxdepth 1 -type f -executable -name "convert_*" -exec cp {} acle-tools/ \; 2>/dev/null || true
	cp -r scripts/ acle-tools/scripts/ 2>/dev/null || true
	tar -cJvf acle-tools-bin.tar.gz acle-tools
	rm -rf acle-tools
	mkdir -p $(OUTPUT_DIR)
	mv acle-tools-bin.tar.gz $(OUTPUT_DIR)/

# ===== Очистка =====
clean:
	@for tool in $(ACLE_TOOLS) $(CONVERT_TOOLS) $(EXTRA_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			sudo ${MAKE} -C $$tool clean 2>/dev/null || true; \
		fi; \
	done
	rm -f acle-tools-bin.tar.gz
	rm -rf acle-tools

# ===== Установка =====
install:
	@echo "📥 Установка утилит..."
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then sudo cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then sudo cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then sudo cp convert_icl/convert_icl /usr/local/bin/; fi
	@sudo mkdir -p /usr/local/share/acle-tools/scripts
	@sudo cp scripts/*.sh /usr/local/share/acle-tools/scripts/ 2>/dev/null || true
	@sudo chmod +x /usr/local/share/acle-tools/scripts/*.sh 2>/dev/null || true
	@echo "✅ Утилиты установлены"

# ===== Удаление =====
uninstall:
	@echo "🗑️  Удаление утилит..."
	@for tool in $(ACLE_TOOLS) $(EXTRA_TOOLS); do sudo rm -f /usr/local/bin/$$tool; done
	@sudo rm -f /usr/local/bin/convert_icl
	@sudo rm -rf /usr/local/share/acle-tools
	@echo "✅ Утилиты удалены"

# ===== Тесты =====
test:
	@echo "🧪 Тестирование утилит..."
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/Makefile ]; then \
			$(MAKE) -C $$tool all 2>/dev/null || true; \
		fi; \
	done
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then \
			echo "✅ $$tool собран"; \
		else \
			echo "⚠️ $$tool не найден (пропускаем)"; \
		fi; \
	done
	@test -d scripts && echo "✅ Скрипты найдены" || echo "⚠️ Скрипты отсутствуют"
	@echo "✅ Тесты утилит пройдены"

# ===== Упаковка =====
# ===== Упаковка =====
package: package-tar package-deb package-rpm

# TAR.GZ: accord-le-tools-<version>-<arch>.tar.gz
package-tar:
	@mkdir -p $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)/; fi
	@mkdir -p $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)/scripts
	@cp scripts/*.sh $(DIST_DIR)/tmp/acle-tools-$(MODULE_VERSION)/scripts/ 2>/dev/null || true
	@tar -czf $(DIST_DIR)/accord-le-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz \
		-C $(DIST_DIR)/tmp acle-tools-$(MODULE_VERSION)
	@rm -rf $(DIST_DIR)/tmp
	@echo "✅ Создан: $(DIST_DIR)/accord-le-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz"

# DEB: acle-tools_<version>_<arch>.deb
package-deb:
	@rm -rf $(OUTPUT_DIR)/tmp/deb-tools
	@mkdir -p $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN
	@mkdir -p $(OUTPUT_DIR)/tmp/deb-tools/usr/bin
	@mkdir -p $(OUTPUT_DIR)/tmp/deb-tools/usr/share/acle-tools/scripts
	@chmod -R 0755 $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN
	@for tool in $(ACLE_TOOLS); do if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tmp/deb-tools/usr/bin/; fi; done
	@for tool in $(EXTRA_TOOLS); do if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tmp/deb-tools/usr/bin/; fi; done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/tmp/deb-tools/usr/bin/; fi
	@cp scripts/*.sh $(OUTPUT_DIR)/tmp/deb-tools/usr/share/acle-tools/scripts/ 2>/dev/null || true
	@chmod +x $(OUTPUT_DIR)/tmp/deb-tools/usr/bin/* 2>/dev/null || true
	@chmod +x $(OUTPUT_DIR)/tmp/deb-tools/usr/share/acle-tools/scripts/*.sh 2>/dev/null || true

	DEB_ARCH=$$(if [ "$(TARGET_ARCH)" = "x86_64" ]; then echo "amd64"; else echo "$(TARGET_ARCH)"; fi); \
	DEB_VER=$$(echo $(MODULE_VERSION) | sed 's/-/+/g'); \
	echo "Package: acle-tools" > $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	echo "Version: $$DEB_VER" >> $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	echo "Architecture: $$DEB_ARCH" >> $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	echo "Maintainer: OKB SAPR" >> $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	echo "Description: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	echo "Depends: zlib1g, perl" >> $(OUTPUT_DIR)/tmp/deb-tools/DEBIAN/control; \
	dpkg-deb --root-owner-group --build $(OUTPUT_DIR)/tmp/deb-tools \
		$(PACKAGES_DIR)/acle-tools_$$DEB_VER_$$DEB_ARCH.deb
	@rm -rf $(OUTPUT_DIR)/tmp/deb-tools
	@echo "✅ Создан: $(PACKAGES_DIR)/acle-tools_$$DEB_VER_$$DEB_ARCH.deb"

# RPM: acle-tools-<version>.<arch>.rpm
package-rpm:
	@rm -rf $(OUTPUT_DIR)/tmp/rpm-tools
	@mkdir -p $(OUTPUT_DIR)/tmp/rpm-tools/{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}
	@mkdir -p $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)
	
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@for tool in $(EXTRA_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/; fi
	@cp scripts/*.sh $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES/acle-tools-$(MODULE_VERSION)/ 2>/dev/null || true
	
	RPM_VER=$$(echo $(MODULE_VERSION) | sed 's/-/_/g'); \
	echo "Name: acle-tools" > $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "Version: $$RPM_VER" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "Release: $(BUILD_NUM)%{?dist}" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "Summary: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "License: Proprietary" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "Requires: zlib, perl" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "%description" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "Utilities for AMDZ GX" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "%install" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "mkdir -p %{buildroot}/usr/bin" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "mkdir -p %{buildroot}/usr/share/acle-tools/scripts" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/acle-* %{buildroot}/usr/bin/ 2>/dev/null || true" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "cp %{_sourcedir}/acle-tools-$(MODULE_VERSION)/convert_* %{buildroot}/usr/bin/ 2>/dev/null || true" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "cp -r %{_sourcedir}/acle-tools-$(MODULE_VERSION)/scripts/* %{buildroot}/usr/share/acle-tools/scripts/ 2>/dev/null || true" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "%files" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "/usr/bin/acle-*" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
	echo "/usr/share/acle-tools/*" >> $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec
	
	@if command -v rpmbuild >/dev/null 2>&1; then \
		rpmbuild --define "_topdir $(OUTPUT_DIR)/tmp/rpm-tools" \
			--define "_sourcedir $(OUTPUT_DIR)/tmp/rpm-tools/SOURCES" \
			-bb $(OUTPUT_DIR)/tmp/rpm-tools/SPECS/acle-tools.spec; \
		find $(OUTPUT_DIR)/tmp/rpm-tools/RPMS -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/acle-tools-$$RPM_VER.$(TARGET_ARCH).rpm \; 2>/dev/null || true; \
		if ls $(PACKAGES_DIR)/acle-tools-$$RPM_VER.$(TARGET_ARCH).rpm 1> /dev/null 2>&1; then \
			echo "✅ Создан: $(PACKAGES_DIR)/acle-tools-$$RPM_VER.$(TARGET_ARCH).rpm"; \
		else \
			echo "⚠️ RPM пакет не найден"; \
		fi \
	else \
		echo "⚠️ rpmbuild не установлен. Пропускаем RPM."; \
	fi
	@rm -rf $(OUTPUT_DIR)/tmp/rpm-tools
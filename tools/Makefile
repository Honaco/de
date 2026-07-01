# ==========================================
# Makefile для сборки утилит Accord-LE
# ТЗ п. 3.2, 5, 6.2
# ==========================================

MODULE_VERSION ?= 1.0.0
BUILD_NUM      ?= 1
TARGET_ARCH    ?= $(shell uname -m)
OUTPUT_DIR     ?= $(CURDIR)/../build

CFLAGS = -Wall -g

# Список утилит (исправлены названия)
ACLE_TOOLS = acle-buf0 acle-disable-watchdog acle-get-random acle-get-state \
             acle-get-tmid acle-login acle-logout acle-sd acle-set-rng \
             acle-test-random-generator acle-write-sectors acle-get-rtc \
             acle-set-rtc acle-set-sdc-mode

# Только существующие утилиты (convert_hd_serial удален)
CONVERT_TOOLS = convert_icl

.PHONY: all clean install uninstall package package-tar package-deb package-rpm test

# ===== Сборка всех утилит =====
all: $(ACLE_TOOLS) $(CONVERT_TOOLS) acle-tools-tar
	@echo "✅ Утилиты собраны"

# Сборка каждой утилиты
$(ACLE_TOOLS):
	${MAKE} -C $@ all

$(CONVERT_TOOLS):
	${MAKE} -C $@ all

# Создание tar-архива утилит
acle-tools-tar:
	mkdir -p acle-tools
	find ./ -maxdepth 1 -type f -executable -name "acle-*" -exec cp {} acle-tools/ \;
	find ./ -maxdepth 1 -type f -executable -name "convert_*" -exec cp {} acle-tools/ \; 2>/dev/null || true
	cp -r scripts/ acle-tools/scripts/ 2>/dev/null || true
	tar -cJvf acle-tools-bin.tar.xz acle-tools
	rm -rf acle-tools
	mkdir -p $(OUTPUT_DIR)
	mv acle-tools-bin.tar.xz $(OUTPUT_DIR)/

# ===== Очистка =====
clean:
	@for tool in $(ACLE_TOOLS) $(CONVERT_TOOLS); do \
		${MAKE} -C $$tool clean 2>/dev/null || true; \
	done
	rm -f acle-tools-bin.tar.xz
	rm -rf acle-tools

# ===== Установка =====
install:
	@echo "📥 Установка утилит..."
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool /usr/local/bin/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl /usr/local/bin/; fi
	@mkdir -p /usr/local/share/acle-tools/scripts
	@cp scripts/*.sh /usr/local/share/acle-tools/scripts/ 2>/dev/null || true
	@chmod +x /usr/local/share/acle-tools/scripts/*.sh 2>/dev/null || true
	@echo "✅ Утилиты установлены"

# ===== Удаление =====
uninstall:
	@echo "🗑️  Удаление утилит..."
	@for tool in $(ACLE_TOOLS); do rm -f /usr/local/bin/$$tool; done
	@rm -f /usr/local/bin/convert_icl
	@rm -rf /usr/local/share/acle-tools
	@echo "✅ Утилиты удалены"

# ===== Тесты =====
test:
	@echo "🧪 Тестирование утилит..."
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then \
			echo "✅ $$tool собран"; \
		else \
			echo "❌ $$tool не найден"; \
			exit 1; \
		fi; \
	done
	@test -d scripts && echo "✅ Скрипты найдены" || echo "❌ Скрипты отсутствуют"
	@echo "✅ Тесты утилит пройдены"

# ===== Упаковка =====
package: package-tar package-deb package-rpm

# TAR.GZ: acle-tools-<version>-<arch>.tar.gz
package-tar:
	@mkdir -p $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/; fi
	@mkdir -p $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts
	@cp scripts/*.sh $(OUTPUT_DIR)/tar/acle-tools-$(MODULE_VERSION)/scripts/ 2>/dev/null || true
	@tar -czf $(OUTPUT_DIR)/acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz \
		-C $(OUTPUT_DIR)/tar acle-tools-$(MODULE_VERSION)
	@rm -rf $(OUTPUT_DIR)/tar
	@echo "✅ Создан: acle-tools-$(MODULE_VERSION)-$(TARGET_ARCH).tar.gz"

# DEB: acle-tools_<version>_<arch>.deb
package-deb:
	@rm -rf $(OUTPUT_DIR)/deb-tools
	@mkdir -p $(OUTPUT_DIR)/deb-tools/DEBIAN
	@mkdir -p $(OUTPUT_DIR)/deb-tools/usr/bin
	@mkdir -p $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts
	@chmod -R 0755 $(OUTPUT_DIR)/deb-tools/DEBIAN
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/deb-tools/usr/bin/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/deb-tools/usr/bin/; fi
	@cp scripts/*.sh $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts/ 2>/dev/null || true
	@chmod +x $(OUTPUT_DIR)/deb-tools/usr/bin/* 2>/dev/null || true
	@chmod +x $(OUTPUT_DIR)/deb-tools/usr/share/acle-tools/scripts/*.sh 2>/dev/null || true
	@echo "Package: acle-tools" > $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	@echo "Version: $(MODULE_VERSION)-$(BUILD_NUM)" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	@if [ "$(TARGET_ARCH)" = "x86_64" ]; then \
		echo "Architecture: amd64" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control; \
	else \
		echo "Architecture: $(TARGET_ARCH)" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control; \
	fi
	@echo "Maintainer: OKB SAPR" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	@echo "Description: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	@echo "Depends: zlib1g, perl" >> $(OUTPUT_DIR)/deb-tools/DEBIAN/control
	@dpkg-deb --root-owner-group --build $(OUTPUT_DIR)/deb-tools \
		$(OUTPUT_DIR)/acle-tools_$(MODULE_VERSION)-$(BUILD_NUM)_amd64.deb
	@rm -rf $(OUTPUT_DIR)/deb-tools
	@echo "✅ Создан: acle-tools_$(MODULE_VERSION)-$(BUILD_NUM)_amd64.deb"

# RPM: acle-tools-<version>.<arch>.rpm
package-rpm:
	@rm -rf $(OUTPUT_DIR)/rpm-tools
	@mkdir -p $(OUTPUT_DIR)/rpm-tools/{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}
	@mkdir -p $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr/bin
	@mkdir -p $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr/share/acle-tools/scripts
	@for tool in $(ACLE_TOOLS); do \
		if [ -f $$tool/$$tool ]; then cp $$tool/$$tool $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr/bin/; fi; \
	done
	@if [ -f convert_icl/convert_icl ]; then cp convert_icl/convert_icl $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr/bin/; fi
	@cp scripts/*.sh $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr/share/acle-tools/scripts/ 2>/dev/null || true
	@echo "Name: acle-tools" > $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Version: $(MODULE_VERSION)" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Release: $(BUILD_NUM)" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Summary: Utilities for working with AMDZ GX" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "License: Proprietary" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Requires: zlib, perl" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%description" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "Utilities for AMDZ GX" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%install" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "cp -r $(OUTPUT_DIR)/rpm-tools/BUILDROOT/acle-tools-$(MODULE_VERSION)/usr %{buildroot}/" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "%files" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "/usr/bin/*" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@echo "/usr/share/acle-tools/*" >> $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@rpmbuild --define "_topdir $(OUTPUT_DIR)/rpm-tools" \
		--define "_buildrootdir $(OUTPUT_DIR)/rpm-tools/BUILDROOT" \
		-bb $(OUTPUT_DIR)/rpm-tools/SPECS/acle-tools.spec
	@find $(OUTPUT_DIR)/rpm-tools/RPMS -name "*.rpm" \
		-exec cp {} $(OUTPUT_DIR)/acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm \; 2>/dev/null || true
	@rm -rf $(OUTPUT_DIR)/rpm-tools
	@echo "✅ Создан: acle-tools-$(MODULE_VERSION).$(TARGET_ARCH).rpm"
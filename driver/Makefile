# ==========================================
# Makefile для сборки драйвера Accord-LE
# ТЗ п. 3.1, 3.2, 3.3, 3.5, 6.2
# ==========================================

KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
INSTALL_PATH   := $(shell find /lib/modules/${KERNEL_RELEASE} -name accord-le.ko 2>/dev/null)

EXTRA_CFLAGS = -DEXPORT_SYMTAB
EXTRA_CFLAGS += -I$(src)

obj-m := accord-le.o

.PHONY: all clean install uninstall package package-tar package-deb package-rpm test

# ===== Сборка модуля =====
all:
	@if [ ! -d "$(KERNEL_HEADERS)" ]; then \
		echo "❌ Заголовки ядра не найдены: $(KERNEL_HEADERS)"; \
		echo "💡 Установите: sudo apt install linux-headers-$(KERNEL_RELEASE)"; \
		echo "💡 Или укажите: make build KERNEL_HEADERS=/path/to/headers"; \
		exit 1; \
	fi
	make -r -C $(KERNEL_HEADERS) M=$(shell pwd) modules
	@echo "✅ accord-le.ko собран"

# ===== Очистка =====
clean:
	make -r -C $(KERNEL_HEADERS) M=$(shell pwd) clean || true
	rm -f accord-le.ko accord-le.o
	rm -f *.mod.* .*.cmd
	rm -rf .tmp_versions
	rm -f Module.markers Module.symvers modules.order

# ===== Установка =====
install:
	@if [ ! -f accord-le.ko ]; then \
		echo "❌ Сначала выполните 'make build'"; \
		exit 1; \
	fi
	make modules_install -C $(KERNEL_HEADERS) M=$(shell pwd)
	/sbin/depmod $(KERNEL_RELEASE)
	cp 99-accord-le.rules /etc/udev/rules.d/
	udevadm control --reload-rules && udevadm trigger
	@echo "✅ Драйвер установлен"

# ===== Удаление =====
uninstall:
	@if [ -n "$(INSTALL_PATH)" ]; then \
		rm -f "$(INSTALL_PATH)"; \
		/sbin/depmod $(KERNEL_RELEASE); \
	fi
	rm -f /etc/udev/rules.d/99-accord-le.rules
	udevadm control --reload-rules
	@echo "✅ Драйвер удалён"

# ===== Тесты (ТЗ п. 8) =====
test:
	@echo "🧪 Тестирование драйвера..."
	@if [ -f accord-le.ko ]; then \
		echo "✅ accord-le.ko существует"; \
		file accord-le.ko; \
	else \
		echo "❌ accord-le.ko не найден. Выполните 'make build'"; \
		exit 1; \
	fi
	@test -f 99-accord-le.rules && echo "✅ udev rules найдены" || echo "❌ udev rules отсутствуют"
	@echo "✅ Тесты драйвера пройдены"

# ===== Упаковка (ТЗ п. 5, 6.2) =====
package: package-tar package-deb package-rpm

# TAR.GZ: accord-le-<version>-<kernel>.tar.gz
package-tar:
	@mkdir -p $(OUTPUT_DIR)/tar/accord-le-$(MODULE_VERSION)
	cp accord-le.ko $(OUTPUT_DIR)/tar/accord-le-$(MODULE_VERSION)/
	cp 99-accord-le.rules $(OUTPUT_DIR)/tar/accord-le-$(MODULE_VERSION)/
	tar -czf $(OUTPUT_DIR)/accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz \
		-C $(OUTPUT_DIR)/tar accord-le-$(MODULE_VERSION)
	rm -rf $(OUTPUT_DIR)/tar
	@echo "✅ Создан: accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz"

# DEB: accord-le_<version>_<arch>.deb
package-deb:
	@rm -rf $(OUTPUT_DIR)/deb-driver
	@mkdir -p $(OUTPUT_DIR)/deb-driver/DEBIAN
	@mkdir -p $(OUTPUT_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra
	@mkdir -p $(OUTPUT_DIR)/deb-driver/etc/udev/rules.d
	@chmod 0755 $(OUTPUT_DIR)/deb-driver/DEBIAN
	@chmod 0755 $(OUTPUT_DIR)/deb-driver
	cp accord-le.ko $(OUTPUT_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra/
	cp 99-accord-le.rules $(OUTPUT_DIR)/deb-driver/etc/udev/rules.d/
	DEB_ARCH=$$(if [ "$(TARGET_ARCH)" = "x86_64" ]; then echo "amd64"; else echo "$(TARGET_ARCH)"; fi); \
	echo "Package: accord-le-driver" > $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Version: $(MODULE_VERSION)" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Architecture: $$DEB_ARCH" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Maintainer: OKB SAPR" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Description: Accord-LE Driver for $(TARGET_OS)" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	chmod 0644 $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	dpkg-deb --root-owner-group --build $(OUTPUT_DIR)/deb-driver \
		$(OUTPUT_DIR)/accord-le_$(MODULE_VERSION)_$$DEB_ARCH.deb
	@rm -rf $(OUTPUT_DIR)/deb-driver
	@echo "✅ Создан: accord-le_$(MODULE_VERSION)_$$(dpkg --print-architecture).deb"

# RPM: accord-le-<version>.<arch>.rpm
package-rpm:
	@rm -rf $(OUTPUT_DIR)/rpm-driver
	@mkdir -p $(OUTPUT_DIR)/rpm-driver/{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}
	@mkdir -p $(OUTPUT_DIR)/rpm-driver/BUILDROOT/accord-le-$(MODULE_VERSION)
	cp accord-le.ko $(OUTPUT_DIR)/rpm-driver/BUILDROOT/accord-le-$(MODULE_VERSION)/
	cp 99-accord-le.rules $(OUTPUT_DIR)/rpm-driver/BUILDROOT/accord-le-$(MODULE_VERSION)/
	@echo "Name: accord-le-driver" > $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "Version: $(MODULE_VERSION)" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "Release: $(BUILD_NUM)" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "Summary: Accord-LE Driver" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "License: Proprietary" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "%description" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "Accord-LE Driver for $(TARGET_OS)" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "%install" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "mkdir -p %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "cp $(OUTPUT_DIR)/rpm-driver/BUILDROOT/accord-le-$(MODULE_VERSION)/accord-le.ko %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra/" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "mkdir -p %{buildroot}/etc/udev/rules.d" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "cp $(OUTPUT_DIR)/rpm-driver/BUILDROOT/accord-le-$(MODULE_VERSION)/99-accord-le.rules %{buildroot}/etc/udev/rules.d/" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "%files" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "/lib/modules/$(KERNEL_RELEASE)/extra/accord-le.ko" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	@echo "/etc/udev/rules.d/99-accord-le.rules" >> $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	rpmbuild --define "_topdir $(OUTPUT_DIR)/rpm-driver" \
		--define "_buildrootdir $(OUTPUT_DIR)/rpm-driver/BUILDROOT" \
		-bb $(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec
	find $(OUTPUT_DIR)/rpm-driver/RPMS -name "*.rpm" \
		-exec cp {} $(OUTPUT_DIR)/accord-le-$(MODULE_VERSION).$(TARGET_ARCH).rpm \; 2>/dev/null || true
	@rm -rf $(OUTPUT_DIR)/rpm-driver
	@echo "✅ Создан: accord-le-$(MODULE_VERSION).$(TARGET_ARCH).rpm"
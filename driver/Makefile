MODULE_VERSION ?= 1.0.0
BUILD_NUMBER ?= 1
TARGET_ARCH    ?= $(shell uname -m)
TARGET_OS      ?= $(shell awk -F= '/^ID=/ {print $$2}' /etc/os-release | tr -d '"')

BUILD_DIR      ?= $(CURDIR)/build
PACKAGES_DIR   ?= $(CURDIR)/packages
DIST_DIR       ?= $(CURDIR)/dist
TESTS_DIR      ?= $(CURDIR)/tests
KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
RPM_VERSION    := $(shell echo $(MODULE_VERSION) | sed 's/-/_/g')
EXTRA_CFLAGS = -DEXPORT_SYMTAB
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -Wno-int-to-pointer-cast

obj-m := accord-le.o
.PHONY: all clean install uninstall package package-tar package-deb package-rpm test ci-build ci-package prepare-dirs

all:
	@if [ ! -d "$(KERNEL_HEADERS)" ]; then \
		echo "Заголовки ядра не найдены: $(KERNEL_HEADERS)"; \
		exit 1; \
	fi
	@make -r -C $(KERNEL_HEADERS) M=$(shell pwd) modules
	@echo "accord-le.ko собран"

ci-build: prepare-dirs all
	@mkdir -p $(BUILD_DIR)
	@cp accord-le.ko $(BUILD_DIR)/
	@echo "Модуль скопирован в $(BUILD_DIR)/"

clean:
	@make -r -C $(KERNEL_HEADERS) M=$(shell pwd) clean
	@rm -f accord-le.ko accord-le.o
	@rm -f *.mod.* .*.cmd
	@rm -rf .tmp_versions
	@rm -f Module.markers Module.symvers modules.order
	@rm -rf $(BUILD_DIR) $(PACKAGES_DIR) $(DIST_DIR) $(TESTS_DIR)

install:
	@if [ ! -f accord-le.ko ]; then \
		echo "accord-le.ko не найден"; \
		exit 1; \
	fi
	@make modules_install -C $(KERNEL_HEADERS) M=$(shell pwd)
	@/sbin/depmod $(KERNEL_RELEASE)
	@if [ -f 99-accord-le.rules ]; then \
		cp 99-accord-le.rules /etc/udev/rules.d/; \
		udevadm control --reload-rules && udevadm trigger; \
	fi
	@echo "Драйвер установлен"

uninstall:
	@INSTALL_PATH=$$(find /lib/modules/${KERNEL_RELEASE} -name accord-le.ko); \
	if [ -n "$$INSTALL_PATH" ]; then \
		rm -f "$$INSTALL_PATH"; \
		/sbin/depmod $(KERNEL_RELEASE); \
	fi
	@rm -f /etc/udev/rules.d/99-accord-le.rules
	@udevadm control --reload-rules
	@echo "Драйвер удалён"

test:
	@if [ -f accord-le.ko ]; then \
		echo "accord-le.ko существует"; \
		file accord-le.ko; \
	else \
		echo "accord-le.ko не найден"; \
		exit 1; \
	fi
	@test -f 99-accord-le.rules
	@echo "Тесты пройдены"

prepare-dirs:
	@mkdir -p $(BUILD_DIR) $(PACKAGES_DIR) $(DIST_DIR) $(TESTS_DIR)

package: prepare-dirs package-tar package-deb package-rpm

ci-package: prepare-dirs package-deb package-rpm

package-tar:
	@if [ ! -f accord-le.ko ]; then \
		echo "accord-le.ko не найден"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)/tar/accord-le-$(MODULE_VERSION)
	@cp accord-le.ko $(BUILD_DIR)/tar/accord-le-$(MODULE_VERSION)/
	@if [ -f 99-accord-le.rules ]; then \
		cp 99-accord-le.rules $(BUILD_DIR)/tar/accord-le-$(MODULE_VERSION)/; \
	fi
	@mkdir -p $(DIST_DIR)
	@tar -czf $(DIST_DIR)/accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz \
		-C $(BUILD_DIR)/tar accord-le-$(MODULE_VERSION)
	@rm -rf $(BUILD_DIR)/tar
	@echo "Создан: $(DIST_DIR)/accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz"

package-deb:
	@if ! command -v dpkg-deb >/dev/null 2>&1; then \
		echo "dpkg-deb не найден"; \
		exit 1; \
	fi
	@if [ ! -f accord-le.ko ]; then \
		echo "accord-le.ko не найден"; \
		exit 1; \
	fi
	@rm -rf $(BUILD_DIR)/deb-driver
	@mkdir -p $(BUILD_DIR)/deb-driver/DEBIAN
	@mkdir -p $(BUILD_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra
	@mkdir -p $(BUILD_DIR)/deb-driver/etc/udev/rules.d
	@chmod 0755 $(BUILD_DIR)/deb-driver/DEBIAN
	@chmod 0755 $(BUILD_DIR)/deb-driver
	@cp accord-le.ko $(BUILD_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra/
	@if [ -f 99-accord-le.rules ]; then \
		cp 99-accord-le.rules $(BUILD_DIR)/deb-driver/etc/udev/rules.d/; \
	fi
	@DEB_ARCH=$$(if [ "$(TARGET_ARCH)" = "x86_64" ]; then echo "amd64"; else echo "$(TARGET_ARCH)"; fi); \
	echo "Package: accord-le-driver" > $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	echo "Version: $(MODULE_VERSION)" >> $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	echo "Architecture: $$DEB_ARCH" >> $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	echo "Maintainer: OKB SAPR" >> $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	echo "Description: Accord-LE Driver for $(TARGET_OS)" >> $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	echo "Depends: kmod, udev" >> $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	chmod 0644 $(BUILD_DIR)/deb-driver/DEBIAN/control; \
	mkdir -p $(PACKAGES_DIR)/debian; \
	dpkg-deb --root-owner-group --build $(BUILD_DIR)/deb-driver \
		$(PACKAGES_DIR)/debian/accord-le_$(MODULE_VERSION)_$$DEB_ARCH.deb
	@rm -rf $(BUILD_DIR)/deb-driver
	@echo "Создан: $(PACKAGES_DIR)/debian/accord-le_$(MODULE_VERSION)_$$DEB_ARCH.deb"

package-rpm:
	@if [ ! -f accord-le.ko ]; then \
		echo "accord-le.ko не найден"; \
		exit 1; \
	fi
	@if ! command -v rpmbuild >/dev/null 2>&1; then \
		echo "rpmbuild не установлен"; \
		exit 1; \
	fi
	@rm -rf $(BUILD_DIR)/rpm-driver
	@mkdir -p $(BUILD_DIR)/rpm-driver/BUILD \
		$(BUILD_DIR)/rpm-driver/RPMS \
		$(BUILD_DIR)/rpm-driver/SOURCES \
		$(BUILD_DIR)/rpm-driver/SPECS \
		$(BUILD_DIR)/rpm-driver/SRPMS \
		$(BUILD_DIR)/rpm-driver/BUILDROOT
	@mkdir -p $(BUILD_DIR)/rpm-driver/SOURCES/accord-le-$(MODULE_VERSION)
	@cp accord-le.ko $(BUILD_DIR)/rpm-driver/SOURCES/accord-le-$(MODULE_VERSION)/
	@if [ -f 99-accord-le.rules ]; then \
		cp 99-accord-le.rules $(BUILD_DIR)/rpm-driver/SOURCES/accord-le-$(MODULE_VERSION)/; \
	fi
	@SPEC="$(BUILD_DIR)/rpm-driver/SPECS/accord-le.spec"; \
	echo "Name: accord-le-driver" > $$SPEC; \
	echo "Version: $(MODULE_VERSION)" >> $$SPEC; \
	echo "Release: $(BUILD_NUMBER)%{?dist}" >> $$SPEC; \
	echo "Summary: Accord-LE Driver" >> $$SPEC; \
	echo "License: Proprietary" >> $$SPEC; \
	echo "Group: System Environment/Kernel" >> $$SPEC; \
	echo "AutoReqProv: no" >> $$SPEC; \
	echo "" >> $$SPEC; \
	echo "%description" >> $$SPEC; \
	echo "Accord-LE Driver for $(TARGET_OS)" >> $$SPEC; \
	echo "" >> $$SPEC; \
	echo "%prep" >> $$SPEC; \
	echo "%build" >> $$SPEC; \
	echo "" >> $$SPEC; \
	echo "%install" >> $$SPEC; \
	echo "mkdir -p %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra" >> $$SPEC; \
	echo "cp %{_sourcedir}/accord-le-$(MODULE_VERSION)/accord-le.ko %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra/" >> $$SPEC; \
	if [ -f 99-accord-le.rules ]; then \
		echo "mkdir -p %{buildroot}/etc/udev/rules.d" >> $$SPEC; \
		echo "cp %{_sourcedir}/accord-le-$(MODULE_VERSION)/99-accord-le.rules %{buildroot}/etc/udev/rules.d/" >> $$SPEC; \
	fi; \
	echo "" >> $$SPEC; \
	echo "%files" >> $$SPEC; \
	echo "/lib/modules/$(KERNEL_RELEASE)/extra/accord-le.ko" >> $$SPEC; \
	if [ -f 99-accord-le.rules ]; then \
		echo "/etc/udev/rules.d/99-accord-le.rules" >> $$SPEC; \
	fi; \
	echo "" >> $$SPEC; \
	echo "%post" >> $$SPEC; \
	echo "/sbin/depmod $(KERNEL_RELEASE)" >> $$SPEC; \
	echo "" >> $$SPEC; \
	echo "%preun" >> $$SPEC; \
	echo "rm -f /lib/modules/$(KERNEL_RELEASE)/extra/accord-le.ko" >> $$SPEC; \
	echo "/sbin/depmod $(KERNEL_RELEASE)" >> $$SPEC; \
	rpmbuild --define "_topdir $(BUILD_DIR)/rpm-driver" \
		--define "_sourcedir $(BUILD_DIR)/rpm-driver/SOURCES" \
		-bb $$SPEC; \
	mkdir -p $(PACKAGES_DIR)/redos; \
	find $(BUILD_DIR)/rpm-driver/RPMS -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/redos/ \;
	@rm -rf $(BUILD_DIR)/rpm-driver
	@echo "пакет создан в $(PACKAGES_DIR)/redos/"
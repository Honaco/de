MODULE_VERSION ?= 1.0.0
BUILD_NUMBER   ?= 1
TARGET_ARCH    ?= $(shell uname -m)
TARGET_OS      ?= $(shell awk -F= '/^ID=/ {print $$2}' /etc/os-release 2>/dev/null | tr -d '"' || echo "debian")
OUTPUT_DIR     ?= $(CURDIR)/../build
KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_VERSION ?= $(KERNEL_RELEASE)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
KERNEL_CONFIG  ?= /boot/config-$(KERNEL_RELEASE)
INSTALL_PATH   := $(shell find /lib/modules/${KERNEL_RELEASE} -name accord-le.ko 2>/dev/null)

# RPM требует замены дефисов на подчеркивания в поле Version spec-файла
RPM_VERSION    := $(shell echo $(MODULE_VERSION) | sed 's/-/_/g')

EXTRA_CFLAGS = -DEXPORT_SYMTAB
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -Wno-int-to-pointer-cast

obj-m := accord-le.o

.PHONY: build clean install uninstall package package-tar package-deb package-rpm test

build:
	@if [ ! -d "$(KERNEL_HEADERS)" ]; then \
		echo "[ERROR] Kernel headers not found: $(KERNEL_HEADERS)"; \
		exit 1; \
	fi
	@$(MAKE) -r -C $(KERNEL_HEADERS) M=$(CURDIR) modules || { echo "[ERROR] Build failed"; exit 1; }
	@echo "[OK] accord-le.ko built"

clean:
	@$(MAKE) -r -C $(KERNEL_HEADERS) M=$(CURDIR) clean 2>/dev/null || true
	@rm -f accord-le.ko accord-le.o
	@rm -f *.mod.* .*.cmd
	@rm -rf .tmp_versions
	@rm -f Module.markers Module.symvers modules.order
install:
	@if [ ! -f accord-le.ko ]; then \
		echo "[ERROR] accord-le.ko not found"; \
		exit 1; \
	fi
	@$(MAKE) modules_install -C $(KERNEL_HEADERS) M=$(CURDIR)
	@/sbin/depmod $(KERNEL_RELEASE)
	@if [ -f 99-accord-le.rules ]; then \
		cp 99-accord-le.rules /etc/udev/rules.d/; \
		udevadm control --reload-rules && udevadm trigger; \
	fi
	@echo "[OK] Driver installed"

uninstall:
	@if [ -n "$(INSTALL_PATH)" ]; then \
		rm -f "$(INSTALL_PATH)"; \
		/sbin/depmod $(KERNEL_RELEASE); \
	fi
	@rm -f /etc/udev/rules.d/99-accord-le.rules
	@udevadm control --reload-rules
	@echo "[OK] Driver uninstalled"

test:
	@if [ -f accord-le.ko ]; then \
		echo "[OK] accord-le.ko exists"; \
		file accord-le.ko; \
	else \
		echo "[ERROR] accord-le.ko not found"; \
		exit 1; \
	fi
	@test -f 99-accord-le.rules && echo "[OK] udev rules found" || echo "[WARN] udev rules missing"
	@echo "[OK] Driver tests passed"

package: package-tar package-deb package-rpm

package-tar:
	@if [ ! -f accord-le.ko ]; then \
		echo "[ERROR] accord-le.ko not found"; \
		exit 1; \
	fi
	@mkdir -p $(OUTPUT_DIR)/tar/accord-le-$(MODULE_VERSION)
	@cp accord-le.ko $(OUTPUT_DIR)/tar/accord-le-$(MODULE_VERSION)/
	@tar -czf $(OUTPUT_DIR)/accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz \
		-C $(OUTPUT_DIR)/tar accord-le-$(MODULE_VERSION)
	@rm -rf $(OUTPUT_DIR)/tar
	@echo "[OK] Created: accord-le-$(MODULE_VERSION)-$(KERNEL_RELEASE).tar.gz"

package-deb:
	@if [ ! -f accord-le.ko ]; then \
		echo "[ERROR] accord-le.ko not found"; \
		exit 1; \
	fi
	@rm -rf $(OUTPUT_DIR)/deb-driver
	@mkdir -p $(OUTPUT_DIR)/deb-driver/DEBIAN
	@mkdir -p $(OUTPUT_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra
	@cp accord-le.ko $(OUTPUT_DIR)/deb-driver/lib/modules/$(KERNEL_RELEASE)/extra/
	@DEB_ARCH=$$(if [ "$(TARGET_ARCH)" = "x86_64" ]; then echo "amd64"; else echo "$(TARGET_ARCH)"; fi); \
	echo "Package: accord-le-driver" > $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Version: $(MODULE_VERSION)" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Architecture: $$DEB_ARCH" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Maintainer: OKB SAPR" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	echo "Description: Accord-LE Driver for $(TARGET_OS)" >> $(OUTPUT_DIR)/deb-driver/DEBIAN/control; \
	dpkg-deb --root-owner-group --build $(OUTPUT_DIR)/deb-driver \
		$(OUTPUT_DIR)/accord-le_$(MODULE_VERSION)_$$DEB_ARCH.deb
	@rm -rf $(OUTPUT_DIR)/deb-driver
	@echo "[OK] Created DEB: accord-le_$(MODULE_VERSION)_$$DEB_ARCH.deb"

package-rpm:
	@if [ ! -f accord-le.ko ]; then \
		echo "[ERROR] accord-le.ko not found"; \
		exit 1; \
	fi
	@if ! command -v rpmbuild >/dev/null 2>&1; then \
		echo "[WARN] rpmbuild not installed. Skipping RPM."; \
		exit 0; \
	fi
	@rm -rf $(OUTPUT_DIR)/rpm-driver
	@mkdir -p $(OUTPUT_DIR)/rpm-driver/BUILD \
		$(OUTPUT_DIR)/rpm-driver/RPMS \
		$(OUTPUT_DIR)/rpm-driver/SOURCES \
		$(OUTPUT_DIR)/rpm-driver/SPECS \
		$(OUTPUT_DIR)/rpm-driver/SRPMS \
		$(OUTPUT_DIR)/rpm-driver/BUILDROOT
	@mkdir -p $(OUTPUT_DIR)/rpm-driver/SOURCES/accord-le-$(MODULE_VERSION)
	@cp accord-le.ko $(OUTPUT_DIR)/rpm-driver/SOURCES/accord-le-$(MODULE_VERSION)/
	
	@SPEC="$(OUTPUT_DIR)/rpm-driver/SPECS/accord-le.spec"; \
	echo "Name: accord-le-driver" > $$SPEC; \
	echo "Version: $(RPM_VERSION)" >> $$SPEC; \
	echo "Release: $(BUILD_NUMBER)%{?dist}" >> $$SPEC; \
	echo "Summary: Accord-LE Driver" >> $$SPEC; \
	echo "License: Proprietary" >> $$SPEC; \
	echo "Group: System Environment/Kernel" >> $$SPEC; \
	echo "%description" >> $$SPEC; \
	echo "Accord-LE Driver for $(TARGET_OS)" >> $$SPEC; \
	echo "%install" >> $$SPEC; \
	echo "mkdir -p %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra" >> $$SPEC; \
	echo "cp %{_sourcedir}/accord-le-$(MODULE_VERSION)/accord-le.ko %{buildroot}/lib/modules/$(KERNEL_RELEASE)/extra/" >> $$SPEC; \
	echo "%files" >> $$SPEC; \
	echo "/lib/modules/$(KERNEL_RELEASE)/extra/accord-le.ko" >> $$SPEC; \
	echo "%post" >> $$SPEC; \
	echo "/sbin/depmod $(KERNEL_RELEASE) || true" >> $$SPEC; \
	rpmbuild --define "_topdir $(OUTPUT_DIR)/rpm-driver" \
		--define "_sourcedir $(OUTPUT_DIR)/rpm-driver/SOURCES" \
		-bb $$SPEC || exit 1; \
	find $(OUTPUT_DIR)/rpm-driver/RPMS -name "*.rpm" -exec cp {} $(OUTPUT_DIR)/ \;
	@rm -rf $(OUTPUT_DIR)/rpm-driver
	@echo "[OK] Created RPM"
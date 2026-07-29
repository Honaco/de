KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
KERNEL_VERSION ?= $(KERNEL_RELEASE)
KERNEL_CONFIG  ?= /boot/config-$(KERNEL_RELEASE)
KERNEL_HEADERS_AVAILABLE := $(shell test -d $(KERNEL_HEADERS) && echo "yes" || echo "no")
GIT_TAG    := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")

GIT_HASH   := $(shell git rev-parse --short HEAD)
BUILD_NUMBER  ?= 1
RAW_VERSION := $(GIT_TAG)-build$(BUILD_NUMBER)-g$(GIT_HASH)
MODULE_VERSION := $(shell echo $(RAW_VERSION) | sed -E 's/^([^0-9])/0.\1/')
TARGET_OS      ?= $(shell grep -oP '(?<=^ID=).+' /etc/os-release)
TARGET_ARCH    ?= $(shell uname -m)

BUILD_DIR      ?= $(CURDIR)/build
DIST_DIR       ?= $(CURDIR)/dist
PACKAGES_DIR   ?= $(CURDIR)/packages
TESTS_DIR      ?= $(CURDIR)/tests
CI_DIR         ?= $(CURDIR)/ci

export KERNEL_HEADERS KERNEL_VERSION KERNEL_CONFIG
export MODULE_VERSION BUILD_NUMBER
export TARGET_OS TARGET_ARCH
export BUILD_DIR DIST_DIR PACKAGES_DIR TESTS_DIR CI_DIR KERNEL_HEADERS_AVAILABLE

.PHONY: all build clean install uninstall package test driver tools help ci-build ci-package ci-test prepare-dirs

all: build

prepare-dirs:
	@mkdir -p $(BUILD_DIR) $(DIST_DIR) $(PACKAGES_DIR) $(TESTS_DIR) $(CI_DIR)

build: prepare-dirs driver tools
	@echo "Сборка завершена"

driver:
	@echo "Сборка драйвера"
	$(MAKE) -C driver all
	@if [ -f driver/accord-le.ko ]; then \
		cp driver/accord-le.ko $(BUILD_DIR)/; \
		echo "Драйвер скопирован в $(BUILD_DIR)/"; \
	else \
		echo "driver/accord-le.ko не найден"; \
		exit 1; \
	fi

tools:
	@echo "Сборка утилит"
	$(MAKE) -C tools all

clean:
	@echo "Очистка"
	$(MAKE) -C driver clean
	$(MAKE) -C tools clean
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(PACKAGES_DIR) $(TESTS_DIR) $(CI_DIR)

install:
	@echo "Установка"
	$(MAKE) -C driver install
	$(MAKE) -C tools install

uninstall:
	@echo "Удаление"
	$(MAKE) -C driver uninstall
	$(MAKE) -C tools uninstall

package: prepare-dirs
	@echo "Упаковка"
	$(MAKE) -C driver package
	$(MAKE) -C tools package
	@echo "Готовые пакеты:"
	@ls -lh $(PACKAGES_DIR)/

test:
	@echo "Тестирование"
	$(MAKE) -C driver test
	$(MAKE) -C tools test

ci-build: prepare-dirs
	@echo "CI сборка"
	$(MAKE) -C driver all
	$(MAKE) -C tools all
	@if [ -f driver/accord-le.ko ]; then \
		cp driver/accord-le.ko $(BUILD_DIR)/; \
	fi
	@echo "Артефакты собраны в $(BUILD_DIR):"
	@ls -lh $(BUILD_DIR)

ci-package: prepare-dirs
	@echo " TARGET_OS=$(TARGET_OS)"
	@echo "PKG_EXT=$(PKG_EXT)"
	@mkdir -p $(PACKAGES_DIR)/$(TARGET_OS)
	
	@if [ "$(PKG_EXT)" = "rpm" ]; then \
		echo "Сборка пакетов для $(TARGET_OS)"; \
		$(MAKE) -C driver package-rpm; \
		$(MAKE) -C tools package-rpm; \
	elif [ "$(PKG_EXT)" = "deb" ]; then \
		echo "Сборка DEB пакетов для $(TARGET_OS)"; \
		$(MAKE) -C driver package-deb; \
		$(MAKE) -C tools package-deb; \
	else \
		echo "PKG_EXT не определен"; \
		exit 1; \
	fi
	
	@echo "Сборка tar.gz архивов"
	@$(MAKE) -C driver package-tar
	@$(MAKE) -C tools package-tar
	
	@echo "Копирование пакетов в $(PACKAGES_DIR)/$(TARGET_OS)"
	@find driver/packages -name "*.deb" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \;
	@find driver/packages -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \;
	@find tools/packages -name "*.deb" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \;
	@find tools/packages -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \;
	@find driver/dist -name "*.tar.gz" -exec cp {} $(DIST_DIR)/ \;
	@find tools/dist -name "*.tar.gz" -exec cp {} $(DIST_DIR)/ \;
	
	@echo "Пакеты собраны в $(PACKAGES_DIR)/$(TARGET_OS):"
	@ls -lh $(PACKAGES_DIR)/$(TARGET_OS)/

ci-test:
	@echo "CI тестирование"
	$(MAKE) -C driver test
	$(MAKE) -C tools test

help:
	@echo "Цели:"
	@echo "  build    - Собрать драйвер и утилиты"
	@echo "  clean   - Очистить артефакты"
	@echo "  install   - Установить в систему"
	@echo "  uninstall  - Удалить из системы"
	@echo "  package   - Создать пакеты"
	@echo "  test     - Запустить тесты"
	@echo "CI цели:"
	@echo "  ci-build  - Сборка для CI"
	@echo "  ci-package - Упаковка для CI"
	@echo "  ci-test  - Тестирование для CI"
	@echo "Параметры:"
	@echo "  KERNEL_HEADERS=/path  - Путь к заголовкам ядра"
	@echo "  MODULE_VERSION=2.1.0  - Версия модуля"
	@echo "  BUILD_NUMBER=15   - Номер сборки"
	@echo "  PKG_EXT=deb|rpm    - Формат пакета"
KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
KERNEL_VERSION ?= $(KERNEL_RELEASE)
KERNEL_CONFIG  ?= /boot/config-$(KERNEL_RELEASE)
KERNEL_HEADERS_AVAILABLE := $(shell test -d $(KERNEL_HEADERS) && echo "yes" || echo "no")
GIT_TAG    := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "1.0.0")
GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_NUMBER  ?= 1
RAW_VERSION := $(GIT_TAG)-build$(BUILD_NUMBER)-g$(GIT_HASH)
MODULE_VERSION := $(shell echo $(RAW_VERSION) | sed -E 's/^([^0-9])/0.\1/')

TARGET_OS      ?= $(shell grep -oP '(?<=^ID=).+' /etc/os-release 2>/dev/null || echo "debian")
TARGET_ARCH    ?= $(shell uname -m)

OUTPUT_DIR     ?= $(CURDIR)/build
DIST_DIR       ?= $(OUTPUT_DIR)/dist
PACKAGES_DIR   ?= $(OUTPUT_DIR)/packages
TESTS_DIR      ?= $(OUTPUT_DIR)/tests
CI_DIR         ?= $(OUTPUT_DIR)/ci

KERNEL_HEADERS_AVAILABLE := $(shell test -d $(KERNEL_HEADERS) && echo "yes" || echo "no")

export KERNEL_HEADERS KERNEL_VERSION KERNEL_CONFIG
export MODULE_VERSION BUILD_NUMBER
export TARGET_OS TARGET_ARCH OUTPUT_DIR
export DIST_DIR PACKAGES_DIR TESTS_DIR CI_DIR KERNEL_HEADERS_AVAILABLE


.PHONY: all build clean install uninstall package test driver tools help ci-build ci-package ci-test

all: build

build: driver tools
	echo "cборка завершена"

driver:
	echo "cборка драйвера"
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver all
	
	if [ -f driver/accord-le.ko ]; then 
		cp driver/accord-le.ko $(OUTPUT_DIR); 
		echo "драйвер скопирован в $(OUTPUT_DIR)"; 
	else 
		echo "driver/accord-le.ko нет"; 
		exit 1; 
	fi

tools:
	echo "сборка утилит"
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C tools all

clean:
	echo "очистка"
	$(MAKE) -C driver clean
	$(MAKE) -C tools clean
	rm -rf $(OUTPUT_DIR)

install:
	echo "установка"
	$(MAKE) -C driver install
	$(MAKE) -C tools install

uninstall:
	echo "удаление"
	$(MAKE) -C driver uninstall
	$(MAKE) -C tools uninstall

package:
	echo "упаковка"
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver package
	$(MAKE) -C tools package
	echo "готовые пакеты:"
	ls -lh $(OUTPUT_DIR)/

test:
	echo "тестирование"
	$(MAKE) -C driver test
	$(MAKE) -C tools test

ci-build:
	echo "ci сборка"
	mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver all
	$(MAKE) -C tools all
	find driver -name "accord-le.ko" -exec cp {} $(OUTPUT_DIR)
	echo " аартефакты собраны в $(OUTPUT_DIR):"
	ls -lh $(OUTPUT_DIR)

ci-package:
	echo " ci cборка пакетов"
	mkdir -p $(PACKAGES_DIR)/$(TARGET_OS)
	$(MAKE) -C driver package
	$(MAKE) -C tools package
	mv $(OUTPUT_DIR)/*.deb $(OUTPUT_DIR)/*.rpm $(OUTPUT_DIR)/*.tar.gz $(PACKAGES_DIR)/$(TARGET_OS)/ 2>/dev/null
	echo "пакеты собраны в $(PACKAGES_DIR)/$(TARGET_OS)"
	ls -lh $(PACKAGES_DIR)/$(TARGET_OS)
	

ci-test:
	echo "ci тестирование..."
	$(MAKE) -C driver test
	$(MAKE) -C tools test

help:
	@echo "Accord-LE Driver Build System"
	@echo "=============================="
	@echo "Цели:"
	@echo "  build      - Собрать драйвер и утилиты"
	@echo "  clean      - Очистить артефакты"
	@echo "  install    - Установить в систему"
	@echo "  uninstall  - Удалить из системы"
	@echo "  package    - Создать пакеты (DEB, RPM, tar.gz)"
	@echo "  test       - Запустить тесты"
	@echo ""
	@echo "CI цели:"
	@echo "  ci-build   - Сборка для CI"
	@echo "  ci-package - Упаковка для CI"
	@echo "  ci-test    - Тестирование для CI"
	@echo ""
	@echo "Параметры:"
	@echo "  KERNEL_HEADERS=/path  - Путь к заголовкам ядра"
	@echo "  MODULE_VERSION=2.1.0  - Версия модуля"
	@echo "  BUILD_NUMBER=15          - Номер сборки"
	@echo "  OUTPUT_DIR=/path      - Директория артефактов"
	@echo ""
	@echo "Пример:"
	@echo "  sudo make build KERNEL_HEADERS=/opt/custom-headers"
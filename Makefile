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
	if [ -f driver/accord-le.ko ]; then \
		cp driver/accord-le.ko $(OUTPUT_DIR); \
		echo "драйвер скопирован в $(OUTPUT_DIR)"; \
	else \
		echo "driver/accord-le.ko нет"; \
		exit 1; \
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
	find driver -name "accord-le.ko" -exec cp {} $(OUTPUT_DIR) \;
	echo " аартефакты собраны в $(OUTPUT_DIR):"
	ls -lh $(OUTPUT_DIR)


ci-package:
	@echo "========================================="
	@echo "[CI-PACKAGE] TARGET_OS=$(TARGET_OS)"
	@echo "[CI-PACKAGE] PKG_EXT=$(PKG_EXT)"
	@echo "========================================="
	@mkdir -p $(PACKAGES_DIR)/$(TARGET_OS)
	
	@# Принудительно вызываем нужные цели в зависимости от PKG_EXT
	@if [ "$(PKG_EXT)" = "rpm" ]; then \
		echo "[INFO] Сборка RPM пакетов для $(TARGET_OS)..."; \
		$(MAKE) -C driver package-rpm || { echo "[ERROR] driver package-rpm failed"; exit 1; }; \
		$(MAKE) -C tools package-rpm || { echo "[ERROR] tools package-rpm failed"; exit 1; }; \
	elif [ "$(PKG_EXT)" = "deb" ]; then \
		echo "[INFO] Сборка DEB пакетов для $(TARGET_OS)..."; \
		$(MAKE) -C driver package-deb || { echo "[ERROR] driver package-deb failed"; exit 1; }; \
		$(MAKE) -C tools package-deb || { echo "[ERROR] tools package-deb failed"; exit 1; }; \
	else \
		echo "[WARN] PKG_EXT не определен или неизвестен, собираем все форматы..."; \
		$(MAKE) -C driver package || true; \
		$(MAKE) -C tools package || true; \
	fi
	
	@# Собираем tar.gz для всех платформ
	@echo "[INFO] Сборка tar.gz архивов..."
	@$(MAKE) -C driver package-tar || true
	@$(MAKE) -C tools package-tar || true
	
	@# Копируем ВСЕ пакеты в директорию артефактов
	@echo "[INFO] Копирование пакетов в $(PACKAGES_DIR)/$(TARGET_OS)..."
	@find $(OUTPUT_DIR) -maxdepth 1 -name "*.deb" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \; 2>/dev/null || true
	@find $(OUTPUT_DIR) -maxdepth 1 -name "*.rpm" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \; 2>/dev/null || true
	@find $(OUTPUT_DIR) -maxdepth 1 -name "*.tar.gz" -exec cp {} $(PACKAGES_DIR)/$(TARGET_OS)/ \; 2>/dev/null || true
	
	@echo "========================================="
	@echo "[OK] Пакеты собраны в $(PACKAGES_DIR)/$(TARGET_OS):"
	@ls -lh $(PACKAGES_DIR)/$(TARGET_OS)/
	@echo "========================================="
	

ci-test:
	echo "ci тестирование"
	$(MAKE) -C driver test
	$(MAKE) -C tools test

help:
	@echo "Цели:"
	@echo "  build      - Собрать драйвер и утилиты"
	@echo "  clean      - Очистить артефакты"
	@echo "  install    - Установить в систему"
	@echo "  uninstall  - Удалить из системы"
	@echo "  package    - Создать пакеты (DEB, RPM, tar.gz)"
	@echo "  test       - Запустить тесты"
	@echo "CI цели:"
	@echo "  ci-build   - Сборка для CI"
	@echo "  ci-package - Упаковка для CI"
	@echo "  ci-test    - Тестирование для CI"
	@echo "Параметры:"
	@echo "  KERNEL_HEADERS=/path  - Путь к заголовкам ядра"
	@echo "  MODULE_VERSION=2.1.0  - Версия модуля"
	@echo "  BUILD_NUMBER=15          - Номер сборки"
	@echo "  OUTPUT_DIR=/path      - Директория артефактов"
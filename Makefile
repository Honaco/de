# Корневой Makefile
# ТЗ п. 3.3: Параметры сборки

KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
KERNEL_VERSION ?= $(KERNEL_RELEASE)
KERNEL_CONFIG  ?= /boot/config-$(KERNEL_RELEASE)

# Версионирование (ТЗ п. 7: tag-buildN-ghash)
GIT_TAG    := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "1.0.0")
GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_NUM  ?= 1
MODULE_VERSION ?= $(GIT_TAG)-build$(BUILD_NUM)-g$(GIT_HASH)

# Целевая платформа
TARGET_OS      ?= $(shell grep -oP '(?<=^ID=).+' /etc/os-release 2>/dev/null || echo "debian")
TARGET_ARCH    ?= $(shell uname -m)

# Директория для артефактов (ТЗ п. 6.1)
OUTPUT_DIR     ?= $(CURDIR)/build

# Экспорт переменных для под-Makefile'ов
export KERNEL_HEADERS KERNEL_VERSION KERNEL_CONFIG
export MODULE_VERSION BUILD_NUM
export TARGET_OS TARGET_ARCH OUTPUT_DIR

# ==========================================
# Цели (ТЗ п. 3.2)
# ==========================================

.PHONY: all build clean install uninstall package test driver tools help

all: build

build: driver tools
	@echo "✅ Сборка завершена. Версия: $(MODULE_VERSION)"
	@echo "📂 Артефакты в: $(OUTPUT_DIR)"

driver:
	@echo "🔨 Сборка драйвера..."
	@mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver all

tools:
	@echo " Сборка утилит..."
	@mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C tools all

clean:
	@echo " Очистка..."
	$(MAKE) -C driver clean
	$(MAKE) -C tools clean
	rm -rf $(OUTPUT_DIR)

install:
	@echo "📥 Установка..."
	$(MAKE) -C driver install
	$(MAKE) -C tools install

uninstall:
	@echo "🗑️  Удаление..."
	$(MAKE) -C driver uninstall
	$(MAKE) -C tools uninstall

package:
	@echo " Упаковка..."
	@mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver package
	$(MAKE) -C tools package
	@echo "📂 Готовые пакеты:"
	@ls -lh $(OUTPUT_DIR)/

test:
	@echo "🧪 Тестирование..."
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
	@echo "Параметры:"
	@echo "  KERNEL_HEADERS=/path  - Путь к заголовкам ядра"
	@echo "  MODULE_VERSION=2.1.0  - Версия модуля"
	@echo "  BUILD_NUM=15          - Номер сборки"
	@echo "  OUTPUT_DIR=/path      - Директория артефактов"
	@echo ""
	@echo "Пример:"
	@echo "  make build KERNEL_HEADERS=/opt/custom-headers"
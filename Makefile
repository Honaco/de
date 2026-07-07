# ==========================================
# Корневой Makefile Accord-LE
# ТЗ п. 3.2, 3.3, 6.1, 7
# ==========================================

KERNEL_RELEASE ?= $(shell uname -r)
KERNEL_HEADERS ?= /lib/modules/$(KERNEL_RELEASE)/build
KERNEL_VERSION ?= $(KERNEL_RELEASE)
KERNEL_CONFIG  ?= /boot/config-$(KERNEL_RELEASE)

# Версионирование (ТЗ п. 7: tag-buildN-ghash)
GIT_TAG    := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "1.0.0")
GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_NUM  ?= 1
# Если GIT_TAG начинается не с цифры, принудительно ставим 0
RAW_VERSION := $(GIT_TAG)-build$(BUILD_NUM)-g$(GIT_HASH)
MODULE_VERSION := $(shell echo $(RAW_VERSION) | sed -E 's/^([^0-9])/0.\1/')

# Целевая платформа
TARGET_OS      ?= $(shell grep -oP '(?<=^ID=).+' /etc/os-release 2>/dev/null || echo "debian")
TARGET_ARCH    ?= $(shell uname -m)

# Директория для артефактов (ТЗ п. 6.1)

OUTPUT_DIR     ?= $(CURDIR)/build
DIST_DIR       ?= $(OUTPUT_DIR)/dist
PACKAGES_DIR   ?= $(OUTPUT_DIR)/packages
TESTS_DIR      ?= $(OUTPUT_DIR)/tests
CI_DIR         ?= $(OUTPUT_DIR)/ci

KERNEL_HEADERS_AVAILABLE := $(shell test -d $(KERNEL_HEADERS) && echo "yes" || echo "no")



# Экспорт переменных для под-Makefile'ов
export KERNEL_HEADERS KERNEL_VERSION KERNEL_CONFIG
export MODULE_VERSION BUILD_NUM
export TARGET_OS TARGET_ARCH OUTPUT_DIR
export DIST_DIR PACKAGES_DIR TESTS_DIR CI_DIR KERNEL_HEADERS_AVAILABLE

# ==========================================
# Цели (ТЗ п. 3.2)
# ==========================================

.PHONY: all build clean install uninstall package test driver tools help ci-build ci-package ci-test

all: build

build: driver tools
	@echo "✅ Сборка завершена. Версия: $(MODULE_VERSION)"
	@echo "📂 Артефакты в: $(OUTPUT_DIR)"

driver:
	@echo "🔨 Сборка драйвера..."
	@mkdir -p $(OUTPUT_DIR)
	$(MAKE) -C driver all
	@# Копируем прямо из текущего места сборки
	@if [ -f driver/accord-le.ko ]; then \
		cp driver/accord-le.ko $(OUTPUT_DIR)/; \
		echo "✅ Драйвер скопирован в $(OUTPUT_DIR)"; \
	else \
		# Если файл в подпапке (как видно из логов /builds/...), ищем его там
		find . -name "accord-le.ko" -exec cp {} $(OUTPUT_DIR)/ \; ; \
		echo "🔍 Поиск и копирование завершены."; \
	fi

tools:
	@echo "🔧 Сборка утилит..."
	@mkdir -p $(OUTPUT_DIR)
	sudo $(MAKE) -C tools all

clean:
	@echo "🧹 Очистка..."
	sudo $(MAKE) -C driver clean
	sudo $(MAKE) -C tools clean
	sudo rm -rf $(OUTPUT_DIR)

install:
	@echo "📥 Установка..."
	sudo $(MAKE) -C driver install
	sudo $(MAKE) -C tools install

uninstall:
	@echo "🗑️  Удаление..."
	sudo $(MAKE) -C driver uninstall
	sudo $(MAKE) -C tools uninstall

package:
	@echo "📦 Упаковка..."
	@mkdir -p $(OUTPUT_DIR)
	sudo $(MAKE) -C driver package
	sudo $(MAKE) -C tools package
	@echo "📂 Готовые пакеты:"
	@ls -lh $(OUTPUT_DIR)/

test:
	@echo "🧪 Тестирование..."
	sudo $(MAKE) -C driver test
	sudo $(MAKE) -C tools test

# ===== CI цели =====


ci-build:
	@echo "🔨 CI сборка: драйвер и утилиты..."
	@mkdir -p $(OUTPUT_DIR)
	# 1. Сборка
	$(MAKE) -C driver all
	$(MAKE) -C tools all
	# 2. Явное копирование драйвера (ищем в подпапках, если нужно)
	@find driver -name "accord-le.ko" -exec cp {} $(OUTPUT_DIR)/ \;
	# 3. Фиксация результата
	@echo "✅ Артефакты собраны в $(OUTPUT_DIR):"
	@ls -lh $(OUTPUT_DIR)

ci-package:
	@echo "📦 Сборка пакетов (CI) для платформы $(TARGET_OS)..."
	@# Создаем структуру согласно ТЗ п. 6.1 с изоляцией дистрибутивов
	@mkdir -p $(PACKAGES_DIR)/$(TARGET_OS)
	$(MAKE) -C driver package
	$(MAKE) -C tools package
	@# Переносим созданные пакеты в изолированную поддиректорию
	@mv $(OUTPUT_DIR)/*.deb $(OUTPUT_DIR)/*.rpm $(OUTPUT_DIR)/*.tar.gz $(PACKAGES_DIR)/$(TARGET_OS)/ 2>/dev/null || true
	@echo "✅ Пакеты для $(TARGET_OS) успешно собраны в $(PACKAGES_DIR)/$(TARGET_OS)"
	@ls -lh $(PACKAGES_DIR)/$(TARGET_OS)
	

ci-test:
	@echo "🧪 CI тестирование..."
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
	@echo "  BUILD_NUM=15          - Номер сборки"
	@echo "  OUTPUT_DIR=/path      - Директория артефактов"
	@echo ""
	@echo "Пример:"
	@echo "  sudo make build KERNEL_HEADERS=/opt/custom-headers"
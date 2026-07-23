## Цели Makefile

Корневой Makefile реализует следующие цели:

make build - Сборка всего проекта

make package - Упаковка пакетов

make install - Установка в систему

make uninstall - Удаление драйвера и утилит из системы

make test - Тестирование

make clean - Очистка

make help - Вывод справки


## Поддерживаемые параметры сборки

Система сборки поддерживает следующие параметры, которые можно передавать через командную строку или задавать в переменных окружения:

| Параметр | Описание | Значение по умолчанию |
|----------|----------|----------------------|
| KERNEL_HEADERS | Путь к заголовкам ядра Linux, необходимым для компиляции драйвера | `/lib/modules/$(KERNEL_RELEASE)/build` |
| KERNEL_VERSION | Версия ядра, для которого собирается драйвер | `$(KERNEL_RELEASE)` (текущееядро) |
| KERNEL_CONFIG | Путь к конфигурационному файлу ядра | `/boot/config-$(KERNEL_RELEASE)` |
| MODULE_VERSION | Версия модуля драйвера | `1.0.0 (или из git tag) |
| BUILD_NUMBER | Номер сборки (инкрементальный) | 1 |
| OUTPUT_DIR | Директория для размещения артефактов сборки | `$(CURDIR)/build` |
| TARGET_OS | Целевая операционная система (для упаковки) | Автоопределение из `/etc/os-release` |
| TARGET_ARCH | Целевая архитектура процессора | `$(uname -m)` |

Примеры использования

Сборка с указанием конкретных заголовков ядра:
``` make build KERNEL_HEADERS=/usr/src/linux-headers-5.4.0-42-generic ```

Сборка с кастомной версией модуля:
``` make package MODULE_VERSION=2.1.0 BUILD_NUMBER=15```

Сборка для другой архитектуры:
``` make build TARGET_ARCH=aarch64 OUTPUT_DIR=/tmp/build-arm64```



## Поддерживаемые платформы

| ОС | Версии | Формат пакетов |
|----|--------|----------------|
| Ubuntu | 20.04, 22.04, 24.04 | DEB |
| Debian | 11, 12 | DEB |
| Astra Linux | Убедить (1.6, 1.7) | DEB |
| ALT Linux | 10, 11 | RPM |

Добавление новой платформы осуществляется декларативно через расширение матрицы сборки в `.gitlab-ci.yml`:
1. Добавление записи в матрицу сборки:
```
.os_matrix:
  parallel:
    matrix:
      # Существующие платформы
      - OS_NAME: "ubuntu"
        OS_IMAGE: "ubuntu:22.04"
        HEADERS_PKG: "linux-headers-generic"
        PKG_EXT: "deb"
        EXTRA_PKGS: "build-essential"

      # Новая платформа
      - OS_NAME: "fedora"
        OS_IMAGE: "fedora:38"
        HEADERS_PKG: "kernel-devel"
        PKG_EXT: "rpm"
        EXTRA_PKGS: "rpm-build gcc make"
```

2. Настройка менеджера пакетов
```
.env_template:
  before_script:
    - |
      if command -v apt-get >/dev/null 2>&1; then
        apt-get update -yqq
        apt-get install -yqq build-essential $HEADERS_PKG $EXTRA_PKGS
      elif command -v dnf >/dev/null 2>&1; then
        dnf install -y gcc make $HEADERS_PKG $EXTRA_PKGS
      elif command -v zypper >/dev/null 2>&1; then
        zypper install -y gcc make $HEADERS_PKG $EXTRA_PKGS
      fi
```

3. Настройка установки пакетов в тестах (В секции test добавьте обработку нового формата пакетов:)
```
test:
  script:
    - |
      if [ "$PKG_EXT" = "deb" ]; then
        dpkg -i $PKG_FILE
        apt-get install -f -y
      elif [ "$PKG_EXT" = "rpm" ]; then
        if command -v dnf >/dev/null 2>&1; then
          dnf localinstall -y $PKG_FILE
        else
          rpm -i $PKG_FILE
        fi
      fi
```


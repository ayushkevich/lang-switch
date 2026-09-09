# lang-switch

Утилита для Linux (X11) на C++: отслеживает смену раскладки клавиатуры и
показывает всплывающее окно с названием новой раскладки по центру нижней трети
экрана. Окно автоматически скрывается через 1 секунду.

## Как это работает

- **Отслеживание раскладки.** `XkbMonitor` открывает собственное соединение с
  X-сервером и подписывается на `XkbStateNotify` события расширения XKB.
  При смене группы клавиатуры приходит уведомление с новым индексом раскладки.
- **Список раскладок.** Читается из root-window свойства `_XKB_RULES_NAMES`
  (строка вида `us,ru`).
- **Отображение.** SDL2 + SDL2_ttf рисуют безрамочное окно с текстом раскладки
  (например, `Русский (ru)`). Окно помечается как `override-redirect`, поэтому
  оконный менеджер его не показывает: приложение отсутствует на панели задач,
  окно не перехватывает фокус и рисуется поверх текущего окна (например,
  браузера). Центр окна — по горизонтали экрана и на отметке 5/6 высоты экрана,
  то есть посередине нижней трети.
- **Таймер.** `PopupController` — чистая конечная машина состояний: окно видно
  ровно 1000 мс; повторная смена раскладки сбрасывает таймер и обновляет текст.

## Зависимости

Debian/Ubuntu/Linux Mint:

```sh
sudo apt install build-essential cmake pkg-config \
    libx11-dev libsdl2-dev libsdl2-ttf-dev
```

GoogleTest подтягивается автоматически через CMake `FetchContent` (нужен доступ
в интернет при конфигурации; при наличии пакета `libgtest-dev` можно использовать его).

## Сборка

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Бинарник: `build/lang-switch`.

## Запуск

```sh
./build/lang-switch
```

Останов — `Ctrl+C` или закрытие окна (SIGTERM тоже обрабатывается).
Отладочные сообщения XKB-монитора: `LANGSWITCH_DEBUG=1 ./build/lang-switch`.

## Установка .deb-пакета (dpkg)

Сборка пакета:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cpack -C Release -G DEB -d build
```

Получится `build/lang-switch-1.1.1-Linux.deb`. Установка:

```sh
sudo dpkg -i build/lang-switch-1.1.1-Linux.deb
# при пропущенных зависимостях:
sudo apt -f install
```

Удаление: `sudo dpkg -r lang-switch`.

Пакет устанавливает:

- `/usr/bin/lang-switch` — саму утилиту;
- `/etc/xdg/autostart/lang-switch.desktop` — автозапуск при старте сессии
  рабочего стола (XDG autostart: приложение стартует в фоне при логине,
  без окна терминала и без записи в панели задач).

После установки сессия подхватит автозапуск при следующем входе; чтобы
запустить прямо сейчас: `/usr/bin/lang-switch &` (или `pkill -x lang-switch`
для остановки).

Отключить автозапуск для конкретного пользователя:

```sh
mkdir -p ~/.config/autostart
cp /etc/xdg/autostart/lang-switch.desktop ~/.config/autostart/
sed -i 's/X-GNOME-Autostart-enabled=true/X-GNOME-Autostart-enabled=false/' \
    ~/.config/autostart/lang-switch.desktop
```

## Тесты

Юнит-тесты (GoogleTest) покрывают чистую логику: разбор списка раскладок,
человеческие имена раскладок, снятие вариантов (`us(dvorak)` -> `us`) и машину
состояний попапа (видимость, авто-скрытие через 1 секунду, сброс таймера,
конвертация XKB-группы).

```sh
ctest --test-dir build --output-on-failure
```

## Структура проекта

```
include/langswitch/
  layout_names.h      разбор имён раскладок (чистая логика)
  popup_controller.h  машина состояний видимости попапа (чистая логика)
  xkb_monitor.h       слушатель XkbStateNotify (X11/XKB)
src/
  layout_names.cpp
  popup_controller.cpp
  xkb_monitor.cpp
  main.cpp            SDL2-приложение: окно, рендер текста, событийный цикл
tests/
  test_layout_names.cpp
  test_popup_controller.cpp
packaging/
  lang-switch.desktop  XDG autostart-ярлык для /etc/xdg/autostart
```

## Ограничения

- Только X11 (Wayland без XWayland-сессии клавиатурные события не перехватывает).
- Имена раскладок мапятся встроенным словарём часто используемых; неизвестные
  показываются как есть (код кода, например `zz`).

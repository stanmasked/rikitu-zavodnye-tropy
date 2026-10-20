# Перенос RIKITU на Nintendo Switch: первый нативный модуль

Браузерный прототип (16 уровней) **не является** Nintendo Switch NRO.

В каталоге `native/` появился платформонезависимый модуль **C99** с проверяемыми
правилами: AABB-столкновения, разрешение прыжка, удар сверху, блокировка
выхода по трём фрагментам и мини-боссу, магазин Моки. Механики и цены
повторяют `game-core.js`; **графические ресурсы и исходные ROM не входят**.

Для локального запуска на ПК:

```sh
gcc -std=c99 -Wall -Wextra -Werror -pedantic -Inative native/rikitu_rules.c native/test_rules.c -o /tmp/rikitu-test-gcc
/tmp/rikitu-test-gcc
clang -std=c99 -Wall -Wextra -Werror -pedantic -Inative native/rikitu_rules.c native/test_rules.c -o /tmp/rikitu-test-clang
/tmp/rikitu-test-clang
```

Тесты являются проверкой **логики правил**, а не игровой петли на Switch.
Дальнейшие необходимые этапы: перенос физики и карт, слой ввода Joy-Con /
Pro Controller, отображение 1280×720 и режимов экрана, звук, сохранения
и настоящая сборка devkitPro/libnx, запуск NRO на целевой платформе.
Утверждённые рисунки Рикиту и Моки нельзя изменять или публиковать
в открытом репозитории без отдельного разрешения.

Не объявлять игру портированной на Switch до сборки и проверки NRO.

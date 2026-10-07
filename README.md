# Heroes V Game API

RU: общий диспетчер поддерживает несколько наблюдателей в HMR и готовых DLL-модах; отдельный девкит для запуска готового мода не нужен. Текущая графическая цепочка проверена в обычном запуске двух отдельных пакетов. EN: Shared observers are verified in native HMR and ordinary same-source player DLL startup with the current graphics chain. See [contract and evidence](docs/mechanisms/game-bindings.md#shared-script-observers--общие-наблюдатели-диспетчера).

## RU

RU: общая C++ библиотека проверенных точек подключения и функций Heroes V Universe. Один источник для SDK, предиктора и справочника. Начальный набор извлечён из работающих проектов; алгоритм расстановки и игровые ресурсы сюда не входят.

Проверка версии и идентичности процесса обязательна; адреса не универсальны для всех сборок. Используйте CMake target heroes5_game_api и закрывайте возвращённый handle. Документация и нативные проверки описаны ниже. Файлы игры библиотека не распространяет.

## EN

Shared header-only C++20 library for the pinned Windows x86 Universe build. It owns build verification, known hook sites and explicit owned-process identity checks. Devkit owns launch/build/HMR orchestration; plugins own gameplay behavior.

## Use / Подключение

Add this repository as `game-api`, then `add_subdirectory(game-api)` and link `heroes5_game_api`. Headers: `h5/build.hpp`, `h5/hooks.hpp`, `h5/process.hpp`. The CMake target supplies headers and bcrypt. Include paths can also be passed directly to MSVC.

RU: сначала VerifyGame проверяет поддержанную сборку. Hook catalog описывает адрес/исходные bytes/источник доказательств, а не устанавливает произвольный detour. OpenOwnedProcess требует PID, creation time и путь из собственного запуска; handle закрывает вызывающий. Несовместимая версия/процесс отклоняется.

EN: Do not treat an address as a stable cross-version API. Do not attach by process name alone. Call-site ABI, thread affinity and lifecycle remain the consumer's obligations. [Contracts and evidence](docs/mechanisms/game-bindings.md).

## Checks / Проверки

```powershell
cmake -S . -B .local/build -A Win32
cmake --build .local/build --config Release
ctest --test-dir .local/build -C Release --output-on-failure
python -X utf8 scripts/docs-check.py --structure-only
```

## Public projects / Публичные проекты

- [Game API](https://github.com/Xaaalera/heroes5-game-api) — this library / эта библиотека.
- [Devkit](https://github.com/Xaaalera/heroes5-mod-devkit) — build, launch, HMR.
- [Deployment Preview](https://github.com/Xaaalera/heroes5-deployment-preview) — predictor / предиктор.
- [Bank Reference](https://github.com/Xaaalera/heroes5-bank-reference) — справочник.
- [Knowledge](https://github.com/Xaaalera/heroes5-knowledge) · [site](https://xaaalera.github.io/heroes5-knowledge/).
- [Xaaalera](https://github.com/Xaaalera) · [email](mailto:dampirsimpl@gmail.com) · [Telegram](https://t.me/Victima).
- [Heroes V Universe / Heroes Lobby](https://h5lobby.com/).

Author projects, not official Universe products. / Проекты автора, не официальные продукты Universe.

## Standalone use / Работа вне мастерской

RU: этот репозиторий можно использовать отдельно. Начни с его README и AGENTS.md; глобальная папка мастерской не обязательна. Если есть .gitmodules, выполни `git submodule update --init --recursive` после клонирования. В связанной мастерской используй её sync-subrepos вместо создания вторых checkout.
EN: This repository can be used independently. Start with its README and AGENTS.md; the global workshop is optional. If .gitmodules exists, initialize pinned dependencies with `git submodule update --init --recursive`. In a linked workshop use its canonical dependency synchronization.

- [Devkit commands / команды SDK](https://github.com/Xaaalera/heroes5-mod-devkit/blob/main/docs/commands.md).
- [Game API contracts / контракты библиотеки](https://github.com/Xaaalera/heroes5-game-api/blob/main/docs/mechanisms/game-bindings.md).
- [Research index / карта исследований](https://xaaalera.github.io/heroes5-knowledge/reference/research-index/).

[Code standards / стандарты кода](https://github.com/Xaaalera/claude-skills).

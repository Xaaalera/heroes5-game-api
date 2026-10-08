# Game API agent instructions

## Documentation ownership and article critique / Документация и критика статей

RU: Единственный источник живой документации — [наш сайт](https://xaaalera.github.io/heroes5-knowledge/), исходники статей — knowledge/docs. В репозиториях оставляем краткий README с назначением и ссылками, AGENTS с рабочими правилами и обязательные лицензии; руководства, справочники и объяснения не копируем по репам. Исторические исследования и приватные журналы сохраняем отдельно, не выдаём за текущую инструкцию. Изменение поведения сопровождается обновлением соответствующей статьи.
EN: The website is the sole source of living human documentation, authored in knowledge/docs. Repositories retain concise README entry points, AGENTS working rules and required notices; manuals, references and explanations link to the site instead of maintaining parallel copies. Preserve historical/private evidence separately and update the canonical article when behavior changes.

RU: Каждая новая или изменённая статья проходит пять независимых критиков по [методу critique](https://github.com/Xaaalera/claude-skills/blob/main/plugins/critique/skills/critique/SKILL.md): понятность новичку, техническая точность, воспроизводимость, структура/навигация, терминология/перевод. Каждый критик указывает место, конкретную проблему, последствия, серьёзность и доказательство; автор не выступает своим критиком. Проверяем факты по коду и наблюдениям, исправляем подтверждённые существенные ошибки, не придумываем оценки. До трёх раундов на статью; фиксируем хеш проверенного текста и решения по замечаниям. Подробный стандарт — [на сайте](https://xaaalera.github.io/heroes5-knowledge/contributing/).
EN: Every new or changed article receives five independent critiques: newcomer clarity, technical accuracy, reproducible steps, structure/navigation, terminology/translation. Findings identify location, failure, consequence, severity and evidence; the author is not a critic. Verify claims against code/observations, fix confirmed substantial defects, record the reviewed content hash and finding disposition, and cap review at three rounds. Follow the canonical site authoring standard; never invent scores.


## Problem-solving order

For every problem, first search our logs, research, backlog and handoff for prior occurrences, attempts, solutions and their verified conditions. Then research the web when needed for causes, documentation, existing solutions and libraries. Only then choose an approach and act. Repeat known experiments only for a new hypothesis or changed conditions. Record links, conclusions and verification limits in the existing log.

RU, 2026-10-06: каждый нативный мод выпускается отдельной DLL. Текущие пакеты используют общие dinput8.dll и d3d9.dll; исходный d3d9.dll игры сохраняется локально как d3d9.universe.dll и не распространяется. Проверены обычный запуск двух пакетов и независимое отключение. Bootstrap и графическая цепочка относятся к запуску; HMR ядра и плагинов проверяется отдельно. Инструкция установки — в README девкита; публикация ещё не завершена.
EN: Each native mod is a separate DLL; current packages share input/graphics infrastructure and retain the original graphics DLL locally. Ordinary two-package startup and independent stop are verified. Startup bootstrap/facade remain separate from hot core/plugin generations. Follow current devkit installation instructions; publication remains pending.


## EN

- Read README.md and docs/mechanisms/game-bindings.md before changes.
- This repository owns shared game bindings and verified identities/signatures. Build/launch/HMR orchestration belongs in devkit; plugin algorithms stay in their plugins.
- Keep one canonical checkout. Workshop consumers use junctions managed by scripts/sync-subrepos.ps1; standalone clones use pinned submodules. Never copy library definitions between consumers.
- Every address/function needs exact build, signature/ABI, caller/thread restrictions and evidence level. Distinguish cataloged legacy evidence, native tests and new live verification. Unknown contracts remain unknown.
- Preserve ordinary-player input restrictions and the owner pause on predictor research. Extracting shared constants is not authorization to rerun predictor campaigns.
- Pair include/h5 changes with docs/mechanisms/game-bindings.md. Preserve dated facts; add corrections instead of rewriting evidence.
- Run native checks and docs-check before finalizing. Never distribute game DLLs/resources, profiles, logs, credentials or personal machine paths.
- Before publication follow CONTRIBUTING.md and .agents/skills/project-review/SKILL.md: five independent reviewers plus the advisory docs audit; never invent scores or attestations.
- New process helpers require explicit owned PID/creation/path validation; never discover a target by name and attach silently. No game launch for unit tests.
- Commit/push requires owner authorization; publication must meet the workspace review rules. Initial local commit is needed to pin this newly requested subrepo.
- Keep RU/EN documentation and these public links current:
  [Game API](https://github.com/Xaaalera/heroes5-game-api),
  [Devkit](https://github.com/Xaaalera/heroes5-mod-devkit),
  [Deployment Preview](https://github.com/Xaaalera/heroes5-deployment-preview),
  [Bank Reference](https://github.com/Xaaalera/heroes5-bank-reference),
  [Knowledge](https://github.com/Xaaalera/heroes5-knowledge),
  [knowledge site](https://xaaalera.github.io/heroes5-knowledge/),
  [author](https://github.com/Xaaalera),
  [email](mailto:dampirsimpl@gmail.com), [Telegram](https://t.me/Victima),
  [Universe](https://h5lobby.com/).

## RU

- Читать README и docs/mechanisms/game-bindings.md. Общие bindings живут здесь, orchestration — в SDK, алгоритмы — в плагинах.
- Один canonical checkout и зависимости без копий; синхронизация мастерской через sync-subrepos.ps1.
- Новые wrappers добавлять только с точной сборкой, сигнатурой, ABI/thread/lifetime контрактом и доказательствами. Unknown не превращать в verified.
- Менять документ механизма вместе с кодом; запускать native/docs checks. Сохранять исходные записи исследований и RU/EN.
- Не распространять игровые ресурсы/бинарники, профили, секреты и пути машины. Подключение только к своему PID с проверкой creation/path/build; unit-тесты игру не запускают.
- Сохранять паузу предиктора и ограничения обычного игрока. Коммиты/публикация — по авторизации владельца и review правилам мастерской.
- Перед публикацией следовать CONTRIBUTING.md и project-review: пять независимых рецензентов и отдельный консультативный аудит документации. Оценки и аттестации не выдумывать.
- Ссылки проектов, автора, email, Telegram и Universe выше поддерживать актуальными во всех потребителях.

## Standalone use / Работа вне мастерской

RU: этот репозиторий можно использовать отдельно. Начни с его README и AGENTS.md; глобальная папка мастерской не обязательна. Если есть .gitmodules, выполни `git submodule update --init --recursive` после клонирования. В связанной мастерской используй её sync-subrepos вместо создания вторых checkout.
EN: This repository can be used independently. Start with its README and AGENTS.md; the global workshop is optional. If .gitmodules exists, initialize pinned dependencies with `git submodule update --init --recursive`. In a linked workshop use its canonical dependency synchronization.

- [Devkit commands / команды SDK](https://xaaalera.github.io/heroes5-knowledge/reference/xkit-commands/).
- [Game API contracts / контракты библиотеки](https://xaaalera.github.io/heroes5-knowledge/reference/game-api/).
- [Research index / карта исследований](https://xaaalera.github.io/heroes5-knowledge/reference/research-index/).

## Code standards / Стандарты кода

RU: перед новой правкой применяй подходящие установленные скиллы из [маркетплейса автора](https://github.com/Xaaalera/claude-skills). Имена переменных/параметров должны объяснять смысл; не использовать непрозрачные сокращения. C++ сохраняет calling convention, lifetime и ABI; Python использует описательные snake_case имена. Обязательные имена API/protocol/register и общепринятые PID/DLL/ABI сокращения допустимы. JS правила не переносить механически на C++/Python.
EN: Load the applicable guides before coding/reviewing. Use descriptive names, small functions with one responsibility, canonical dependencies and no speculative abstractions. Preserve native ABI/protocol compatibility during readability changes. Existing code is changed when relevant, not mass-renamed by this policy.

- All code: [solid](https://github.com/Xaaalera/claude-skills/blob/main/plugins/meta/skills/solid/SKILL.md), [ockham](https://github.com/Xaaalera/claude-skills/blob/main/plugins/meta/skills/ockham/SKILL.md).
- JS/TS only: [conventions](https://github.com/Xaaalera/claude-skills/blob/main/plugins/frontend-js/skills/conventions/SKILL.md).
- Tests: Codex alias `tests-architecture`, upstream [tests:architecture](https://github.com/Xaaalera/claude-skills/blob/main/plugins/tests/skills/architecture/SKILL.md).
- Documents: [standard](https://github.com/Xaaalera/claude-skills/blob/main/plugins/docs/skills/standard/SKILL.md), [lean-writing](https://github.com/Xaaalera/claude-skills/blob/main/plugins/meta/skills/lean-writing/SKILL.md), [wittgenstein](https://github.com/Xaaalera/claude-skills/blob/main/plugins/meta/skills/wittgenstein/SKILL.md).
- Changed user-facing UI: [ui-strings](https://github.com/Xaaalera/claude-skills/blob/main/plugins/i18n/skills/ui-strings/SKILL.md), [responsive-layout](https://github.com/Xaaalera/claude-skills/blob/main/plugins/frontend-css/skills/responsive-layout/SKILL.md).
- New public JSON error boundaries: [format](https://github.com/Xaaalera/claude-skills/blob/main/plugins/error/skills/format/SKILL.md); version changes explicitly, do not silently reinterpret native status words.
- Reviewers load every applicable guide listed in .claude/review.config.json. If a guide is not installed, read the canonical source above and report availability honestly. Do not vendor independent copies of these standards.

## Human-usable functionality / Использование человеком

RU/EN: Human binding documentation names functions/events/types and describes their role; no raw memory addresses, structure byte offsets or disassembler address labels. Numeric bindings remain in code/private evidence. Reviewers reject address-only explanations and require meaningful symbols.

RU/EN: infrastructure must prefer standard language facilities or maintained libraries. Add domain adapters only for missing project contracts; do not invent another log engine. Diagnostic records need operation/plugin/stage/level/time context and actionable source locations. Raw details belong in linked local artifacts.

RU: весь функционал проекта должен быть пригоден для самостоятельного использования человеком без AI. Основной сценарий требует понятного входа, справки, разумных настроек по умолчанию, видимого состояния и ошибок с действием для исправления. Цепочка внутренних Python/PowerShell/RPC команд не заменяет пользовательский интерфейс. Разработчик должен уметь подготовить окружение, создать/запустить/обновить плагин и получить готовый мод по документации самостоятельно.
EN: Every feature must be usable by a person without an AI agent. Provide a clear entry point, help, sensible defaults, observable progress and actionable errors. Internal scripts/RPC sequences may support diagnostics but do not satisfy the main user workflow. Acceptance includes following the documented workflow as a human; never document a planned friendly command as already implemented. This is a project rule, not a new skill.

RU: правило также относится к README, документации, справке, описаниям, примерам и сообщениям. Писать для указанной аудитории простым языком: зачем функция нужна, как начать, какой результат ожидается, как исправить ошибку. Объяснять термины при первом использовании; внутренние механизмы выносить в документацию разработчика. Инструкция не должна требовать AI для расшифровки или поиска пропущенных шагов.
EN: Apply the same rule to README, documentation, help, descriptions, examples and messages. Explain purpose, starting steps, expected result and recovery in language appropriate to the reader. Define unfamiliar terms on first use; keep internals in developer documentation. A person must be able to follow the instructions without AI filling missing steps.

# Contributing

## RU

Это инструкция для изменения самой библиотеки. Готовому моду Node.js и инструменты проверок не нужны. Для локальных проверок репозитория установи Node.js22+, Python3.10+ и MSVC x86/CMake, затем `npm ci`: зависимости проверок и Git hook настроятся автоматически.

Перед публикацией: `npm test`, `npm run check:docs`, `npm run check`, `npm run review:secrets`. Пять независимых read-only ревьюеров: craft, architecture, tests, docs, security. Контракты лежат в .agents/review; порядок — в .agents/skills/project-review/SKILL.md. Не заменять их самопроверкой автора. Docs-review проверяет purpose, structure, specificity, reproducibility, evidence, applicability, translations, maintenance.

Оценка:10−20×blocker−3×major−minor; нерешённые blocker/major запрещены. После исправлений проверяется текущий точный diff. Фактические результаты записать через `npm run review:attest -- results.json`, отдельно закоммитить запись и выполнить `npm run review:gate`. Новые правки требуют свежего ревью. Git hook блокирует непроверенный push; CI запускает машинные проверки, не моделей. Обход локального hook технически возможен; серверная защита не обещается без отдельной настройки.

## EN

This guide is for library maintainers. Ready-made mods do not need Node.js or review tools. Local repository checks need Node.js22+, Python3.10+ and MSVC x86/CMake; `npm ci` installs check dependencies and the Git hook.

Before publication run test/docs/check/secrets and independent craft, architecture, tests, docs and security reviews using the contracts and project-review skill. Score10−20×blockers−3×majors−minors; no unresolved blocker/major. Docs review covers all eight criteria listed above. Record actual results through review:attest, commit the record separately and run review:gate for the exact reviewed diff. Changes invalidate review. CI validates machine checks and records, not model identity; local hooks can be bypassed and server enforcement is separate.

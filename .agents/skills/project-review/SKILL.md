---
name: project-review
description: Review this Heroes V mod workspace before a commit is pushed, using independent code, architecture, tests, documentation and security lenses, then record a passing attestation only for the exact reviewed diff.
---

# Project review for Codex

## EN

Use `.claude/review.config.json` and the contracts in `.agents/review/`.
These contracts were adapted from Xaaalera/claude-skills revision c8fec7777eb8e79c1e905be6d3296188c213870d.
Claude-specific tools and model settings do not apply; use the available Codex tools and inherited model.

1. Review committed work: `npm run review:info --silent` returns base, hash and enabled lenses. If the worktree has relevant uncommitted edits, commit the authorized change before reviewing. Never silently omit those edits.
2. Run every configured deterministic gate and `npm run review:secrets -- --base <base>`. A nonzero result blocks attestation.
3. For each enabled lens, use a separate read-only subagent with its `.agents/review/review-<name>.md` contract, project config, the exact base..HEAD diff and machine-check results. Limit searches to tracked project files and the explicitly referenced installed skill. Do not scan the game, profiles or extracted assets for a code review.
4. Each lens reports concrete findings with severity, location, evidence and an input/action/wrong-result scenario. No invented findings, scores or claimed agent runs. If delegation is unavailable, say review remains pending; do not substitute a claimed independent pass.
5. Score each lens as `max(0, 10 - 20*blockers - 3*majors - minors)`. Meet its configured threshold, with no unresolved Blocker or Major. Check semantic parity of RU/EN project documentation. Show all findings, including disagreements; resolve them against evidence. Fixes belong to implementation, not the read-only reviewers.
6. Run `.agents/review/docs-auditor.md` separately over the declared non-frozen documentation layers. This is an advisory whole-tree audit, not another scored lens.
7. After fixes, review the final diff with every enabled lens. Stop automatic fix/review cycling after three attempts and report unresolved issues; never manufacture a pass.
8. Recheck that the hash has not changed. Write the actual `{ "craft": {"score": 10, "verdict": "PASS"}, ... }` results to a temporary JSON file. Use the package API or pass serialized JSON correctly for the host shell; no manually written attestations.
9. Run `npm run review:attest -- <results.json>`; the project adapter validates the configured thresholds/gates and calls the pinned package's attestation API. Commit only the resulting `.review/attestations/` change, and run `npm run review:gate`. No source edit may occur between the final pass and its attestation. All project commands default to `origin/main`, not the feature branch upstream; use the same explicit `--base <commit>` everywhere if overriding it.

The attestation records agent judgments; it is not a cryptographic proof of reviewer independence. GitHub CI runs deterministic checks and validates the record, not AI reviewers. No API key is required for this local-agent workflow.

## RU

Конфигурация и контракты указаны в EN и общие для обоих языков. Использовать доступные инструменты Codex и унаследованную модель; настройки Claude не применяются.

1. Получить base/hash/линзы через review:info. Проверять закоммиченные изменения; авторизованные незакоммиченные правки сначала включить в коммит, не исключать молча.
2. Выполнить все gates и secret scan для той же базы. Любая ошибка блокирует аттестацию.
3. Каждой включённой линзе дать отдельного независимого read-only агента, её контракт, конфигурацию, точный diff и результаты проверок. Искать только в отслеживаемых файлах и указанных skills, не в игре или профилях.
4. Требовать конкретные находки с severity, местом, доказательством и сценарием. Не выдумывать запуски и оценки. Без делегирования независимое ревью остаётся незавершённым.
5. Применять общую формулу EN и пороги конфигурации; не оставлять нерешённые Blocker/Major. Проверять согласованность RU/EN, показывать разногласия и разрешать по доказательствам. Исправляет основной агент, не рецензенты.
6. Отдельно запускать docs-auditor по всем актуальным объявленным слоям. Это рекомендация без числовой оценки.
7. После исправлений всем линзам проверить окончательный diff. Максимум три автоматических цикла; затем сообщить проблемы без фиктивного PASS.
8. Проверить неизменность hash, записать реальные оценки JSON во временный файл. Использовать API закреплённого пакета или корректную сериализацию для shell.
9. Записать через `npm run review:attest -- <results.json>`: адаптер проверит пороги/gates и вызовет API закреплённого пакета. Отдельно закоммитить запись и запустить review:gate. После PASS не менять код до записи. Общая база команд — origin/main, не upstream feature-ветки; при переопределении передавать одинаковый --base всюду.

Аттестация — журнал решения, не криптографическое доказательство независимости. CI выполняет детерминированные проверки, не AI-ревью. Локальному процессу API key не требуется.

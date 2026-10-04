# Repository review

## RU

Пять независимых линз проверяют код, архитектуру, тесты, документацию и безопасность. Установка через npm ci включает локальный pre-push hook. Все конфигурации и контракты входят в репозиторий; работа с готовым модом не требует этих инструментов.

## EN

Five independent lenses assess craft, architecture, tests, docs and security. npm ci installs dependencies and the pre-push hook. Configuration/contracts are included; player use does not depend on this tooling.

## Purpose
Bind publication to actual independent review of the selected Git diff plus deterministic checks.

## Operation
review:info reports base/hash and Markdown paths. Reviewers read their .agents/review contracts; project-review coordinates read-only assessments. review:attest validates actual results, documentation criteria, secrets and native/docs gates. The pre-push hook calls review:gate; GitHub workflow runs checks using pinned actions and package-lock dependencies. The initial library source at dc8d775 is also reviewed as a full-tree baseline, rather than silently assumed covered by a later setup-only diff.

## Limits
Attestations record judgments, not cryptographic identities. CI does not run models or the game. A local hook can be bypassed; remote branch policy is separate. Reject dirty source before attesting. C++ tests validate synthetic boundaries, not live-game behavior.

## Verification
Before first publication demonstrate rejection without a valid attestation, run configured tests, obtain all five real independent results and recheck the final hash. The docs auditor is an additional unscored pass over declared live docs. No fabricated results or manually forged attestations.

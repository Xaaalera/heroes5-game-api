# Repository review

## RU

Пять независимых линз проверяют код, архитектуру, тесты, документацию и безопасность. Установка через npm ci включает локальный pre-push hook. Все конфигурации и контракты входят в репозиторий; работа с готовым модом не требует этих инструментов.

## EN

Five independent lenses assess craft, architecture, tests, docs and security. npm ci installs dependencies and the pre-push hook. Configuration/contracts are included; player use does not depend on this tooling.

## Purpose
Bind publication to actual independent review of the selected Git diff plus deterministic checks.

The configured docs command checks declared code/document pairs as well as structure; --structure-only is a local bootstrap diagnostic, not the publication check.

RU: публикация требует фактического независимого ревью выбранного Git diff и машинных проверок.

Настроенная команда docs проверяет пары код/документ и структуру. --structure-only служит локальной диагностикой при создании репозитория, а не проверкой публикации.

## Operation
review:info reports base/hash and Markdown paths. Reviewers read their .agents/review contracts; project-review coordinates read-only assessments. review:attest validates actual results, documentation criteria, secrets and native/docs gates. The pre-push hook calls review:gate; GitHub workflow runs checks using pinned actions and package-lock dependencies. The initial library source at dc8d775 is also reviewed as a full-tree baseline, rather than silently assumed covered by a later setup-only diff.

RU: review:info возвращает base/hash и Markdown пути. Рецензенты используют контракты .agents/review; project-review организует независимое чтение. review:attest проверяет реальные результаты, критерии документации, секреты и native/docs проверки. Pre-push вызывает review:gate; CI использует закреплённые actions и package-lock. Начальный код dc8d775 дополнительно проверяется целиком: он не считается проверенным лишь из-за последующего diff настройки. Внутренние контракты агентов написаны на английском; полный порядок для разработчика дан на русском и английском в project-review/CONTRIBUTING.

## Limits
Attestations record judgments, not cryptographic identities. CI does not run models or the game. A local hook can be bypassed; remote branch policy is separate. Reject dirty source before attesting. C++ tests validate synthetic boundaries, not live-game behavior.

RU: аттестация хранит оценки, не криптографическое доказательство личности. CI не запускает модели/игру. Локальный hook обходится; серверные правила отдельно. Незакоммиченный код блокирует аттестацию. C++ тесты проверяют искусственные границы, не живую игру.

## Verification
Before first publication demonstrate rejection without a valid attestation, run configured tests, obtain all five real independent results and recheck the final hash. The docs auditor is an additional unscored pass over declared live docs. No fabricated results or manually forged attestations.

RU: до первой публикации подтвердить отказ без аттестации, выполнить тесты, получить пять независимых результатов и сверить итоговый hash. Аудитор документации делает дополнительную проверку без оценки. Результаты/аттестации не подделывать. Регрессия review-gate.test.mjs проверяет отказ ниже порога и успешный проход ровно на пороге через настоящий adapter в отдельном тестовом репозитории.

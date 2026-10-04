# Game API agent instructions

## EN

- Read README.md and docs/mechanisms/game-bindings.md before changes.
- This repository owns shared game bindings and verified identities/signatures. Build/launch/HMR orchestration belongs in devkit; plugin algorithms stay in their plugins.
- Keep one canonical checkout. Workshop consumers use junctions managed by scripts/sync-subrepos.ps1; standalone clones use pinned submodules. Never copy library definitions between consumers.
- Every address/function needs exact build, signature/ABI, caller/thread restrictions and evidence level. Distinguish cataloged legacy evidence, native tests and new live verification. Unknown contracts remain unknown.
- Preserve ordinary-player input restrictions and the owner pause on predictor research. Extracting shared constants is not authorization to rerun predictor campaigns.
- Pair include/h5 changes with docs/mechanisms/game-bindings.md. Preserve dated facts; add corrections instead of rewriting evidence.
- Run native checks and docs-check before finalizing. Never distribute game DLLs/resources, profiles, logs, credentials or personal machine paths.
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
- Ссылки проектов, автора, email, Telegram и Universe выше поддерживать актуальными во всех потребителях.

# Game bindings / Привязки к игре

## RU

Библиотека централизует SHA-256/отпечаток четырёх бинарников Universe, сигнатуры известных hooks и проверку своего процесса. VerifyGame не пишет файлы; OpenOwnedProcess требует PID/creation/path собственного запуска, проверяет сборку, закрывает handle при отказе. Успешный handle закрывает вызывающий.

Каталог ниже перенесён из справочника, предиктора и SDK. Это прежние доказательства, не новая живая проверка каждого адреса. Calling convention, поток, сохранение регистров и lifetime остаются обязанностью потребителя. Описаний неизвестных структур и непроверенных wrappers пока нет. Нативный тест проверяет SHA, отказ неверной сборки/creation и адреса возврата/CALL target.

## EN

## Purpose
RU: переиспользовать знания двух плагинов и SDK без копирования адресов/проверок. EN: One source for validated build fingerprints, known hook sites and owned-process guards.

## Operation
`build.hpp` owns SHA-256 and the four-binary Universe fingerprint formerly in devkit/player_launch.hpp. VerifyGame rejects wrong names/hashes without writing files. `hooks.hpp` provides typed fixed-size signatures and resume addresses. `process.hpp` opens only the explicit PID and verifies creation time, executable path and build; failures close the handle.

| Binding | Origin / источник | Evidence / доказательство |
|---|---|---|
| BankLayout 0x5f8800 | bank-reference InstallSelector | Existing native selector test and live bank-window research |
| PlacementRenderer 0x577010 | deployment-preview InstallRendererHook | Legacy compiled hook and retained live predictor research |
| AdventureAttack 0x433440 | deployment-preview InstallRendererHook | Legacy pre-battle hook |
| PlacementHover 0x5767a0 | deployment-preview InstallRendererHook | Legacy hover hook |
| ScriptDispatchCall 0xd112db → 0xd10980 | devkit game_control/HMR | Live CALL replacement/rollback plus machine-state test |

These facts are extracted from existing source; catalog extraction does not create new accuracy or live coverage. Full evidence history: [research diary](https://xaaalera.github.io/heroes5-knowledge/reference/research-diary/). Native package game files are never bundled.

## Limits
Windows x86 and the exact hashed Universe build only. Signatures identify sites, not arbitrary-safe detour recipes. No inferred function prototypes/structure offsets are exposed yet. Consumers must preserve registers, stack, FPU, thread affinity and callback lifetime. OpenOwnedProcess validates supplied provenance; the caller is responsible for owning the launch and closing the returned handle. It does not authorize attaching to an owner's unrelated game.

RU: расширять каталог по мере подтверждения. Typed wrappers и layouts добавлять только после доказанной сигнатуры, calling convention, границ lifetime и теста. Непроверенные гипотезы хранить в research, не выдавать за готовый API.

## Verification
Native test: known SHA vector, corrupt correctly-named H5_Game.exe, wrong process creation, wrong path with correct creation, failed-open handle cleanup, hook resume addresses and decoded CALL target. Consumers compile against the canonical headers. Existing source/DLL accuracy evidence remains tied to its original hashes; extracting constants does not transfer it to a rebuilt predictor.

RU: тест теперь доходит до проверки хеша правильно названного H5_Game.exe и пути процесса с правильным creation time; также сверяет отсутствие утечки handle при отказе. Проверка имени файла не подменяет проверку хеша.

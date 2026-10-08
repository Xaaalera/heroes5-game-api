# Game bindings / Привязки к игре

## Adventure keyboard capture / Захват клавиатуры на карте

RU: новый локальный механизм перехватывает подтверждённое событие нажатия до игровых обработчиков карты. При видимом окне консоли с фокусом это событие считается обработанным; при скрытой консоли выполняется исходный обработчик. Служебные, периодические и неизвестные события проходят без изменений. Публичные функции: `CreateAdventureInputGate` и `FindAdventureInputGate`. Текущее окно консоли определяется при каждом вызове по свойству окна игры.

EN: The new local mechanism gates verified key-press events before adventure gameplay handlers. A visible focused console consumes them; a hidden console preserves original dispatch. Tick, named and unknown events pass unchanged. The functions above create and validate the resident gate. Each invocation resolves the current console property; no worker-owned HWND/thread pair is retained.

RU: код и данные принадлежат процессу игры; в них нет callback ядра SDK. Системные вызовы разрешаются через user32. Нативная проверка покрывает настоящий HWND, фокус, скрытие, неизвестные события и удаление свойства консоли. Живая проверка подтвердила блокировку I в консоли и открытие снаряжения после её закрытия, включая автоматическую замену окна консоли и ядра в том же процессе. Обработчик сохранился, окно изменилось, исходники восстановлены и игра закрылась штатно. Выпуск, все виды мышиного ввода, удерживаемые клавиши и все экраны игры этим не подтверждены.

EN: Code and POD data belong to the game process and retain no SDK callback; system targets resolve through user32. Native checks exercise a real HWND, focus, hiding, unknown events and console property removal. Live checks verify I suppression in console and equipment opening after hide, including automatic console HWND and core replacement in the same process. The resident gate persists, sources are restored and the game exits normally. Release readiness, held-key transitions, every mouse event and every game screen remain outside this evidence.

## Graphics repair verification / Проверка исправления графики

RU: После исправления и после отката библиотека повторно читает изменённый участок и проверяет восстановленную защиту страницы и принадлежность образу. Неподтверждённые байты или защита запрещают продолжать запуск. Контракт остановленных потоков собственного процесса сохраняется; эта проверка не разрешает исправление во время игры.

EN: Repair and rollback now read back the changed bytes and verify restored page protection and image ownership. Unconfirmed bytes or protection prevent launch continuation. The owned-process stopped-thread contract remains required; this change does not authorize live-session repair. Cache flushing follows [Microsoft's executable-memory requirements](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotectex).

RU: Нативные проверки вызывают ту же внутреннюю процедуру записи и отката с контролируемыми операциями памяти. Десять сценариев покрывают успех, частичную запись, ошибки защиты, очистки кеша и чтения, а также сбои каждого этапа отката. Это проверка логики транзакции; работа Windows API с живым образом игры проверяется отдельно.

EN: Ten native scenarios exercise the production transaction with controlled memory operations: success, partial write, protection/flush/readback failures, and failure at each rollback stage. These prove transaction decisions, not actual Windows memory operations against the live game image.

RU: Проверка и исправление образа выделены во внутреннюю процедуру, которая заимствует handle процесса и не закрывает его. Публичная функция сохраняет проверки PID, времени создания, пути и сборки, исходное имя библиотеки и закрытие собственного handle. Оболочка SDK вызывает внутреннюю процедуру только при первой инициализации свежей проверенной оригинальной DLL, до создания графических объектов и передачи фабрики игре. Проверены меню и штатное закрытие на текущей Windows; полная приёмка SDK ещё не завершена.

EN: The internal image routine borrows its process handle without closing it. The public wrapper retains PID/creation/path/build checks, the original library path and owned-handle cleanup. The SDK facade calls it only during first initialization of a fresh pinned original, before graphics objects exist or the factory is published. Current-OS menu/normal-close evidence is verified; full SDK acceptance remains open. This grants no mid-session or arbitrary concurrent repair permission.

## Graphics chain identity / Проверка графической цепочки

RU: `VerifyOwnedImagePlacement` также принимает необязательный доверенный хеш оболочки и передаёт его проверке собственного процесса. По умолчанию разрешена только исходная графическая цепочка. Проверки файла, отображения, адреса и доступного диапазона библиотеки анимации сохраняются.

EN: `VerifyOwnedImagePlacement` may receive the sealed caller's explicit facade hash and forwards it to owned-process verification. Default acceptance remains the original graphics chain. Its image-file identity, mapping, placement and required readable-range checks remain unchanged.

RU: `VerifyGraphicsFiles` по умолчанию принимает только исходную графическую библиотеку Universe. Для цепочки с отложенной загрузкой вызывающая сторона явно передаёт SHA-256 оболочки из своей проверенной сборки SDK. Тогда дополнительно требуется неизменённая исходная библиотека `d3d9.universe.dll`. Хеш, записанный в папке игры сторонним файлом, разрешения не даёт.

RU: `VerifyGame` сохраняет проверки EXE и библиотек Universe; `OpenOwnedProcess` сохраняет собственный PID, время создания и путь процесса. Новая цепочка не включается автоматически. Проверки SDK и готовых модов должны передать её доверенный хеш явно. Живые меню, карта и две отдельные DLL подтверждены только диагностической сборкой; основной выпуск ещё требует интеграции и повторной проверки.

EN: Graphics identity defaults to the original pinned Universe wrapper. A caller may explicitly supply the facade SHA-256 from its sealed SDK build; this requires the byte-identical original at d3d9.universe.dll as well. Untrusted game-folder metadata grants no permission. VerifyGame retains the other game pins, and OpenOwnedProcess retains PID/creation/path guards. The default remains strict. Current diagnostic menu/map/two-DLL proofs do not certify canonical packaging, editor or lifetime repair.

## Graphics proxy lifetime / Время жизни графического устройства

RU: `RepairOwnedGraphicsProxyLifetime` исправляет преждевременное удаление обёртки устройства в поддерживаемой графической библиотеке Universe. При оставшихся ссылках устройство сохраняется; при нуле ссылок исходный путь освобождения сохраняется. Проверяются собственный PID, время создания, путь и сборка игры, файл загруженной библиотеки, отображение образа и полная сигнатура изменяемого участка. Файлы игры не меняются. Вызывать только при остановленных потоках собственного процесса, до инициализации графики; текущий потребитель — событие загрузки библиотеки в запускателе SDK. Повторное применение распознаётся. При ошибке выполняется откат; неподтверждённый откат запрещает продолжать запуск.

EN: Startup-only compatibility binding for the pinned Universe graphics proxy. Keep the wrapper while references remain; preserve its original final-release path. Requires explicit owned process provenance and a quiescent loader boundary. Verifies process/build, retained image file identity/hash, mapped image and complete instruction signature. Changes process memory only, without executable allocation or callbacks. Repeated application is recognized. Failed writes attempt rollback; the caller must not resume after unconfirmed recovery.

RU/EN, 2026-10-06: fresh SDK-environment diagnostic baseline captured positive-reference deletion followed by same-wrapper conditional use and an access violation. The conditional candidate preserved that wrapper and later freed it at zero references. Actual Release emulation covers zero/positive/max count and ABI; native tests cover ownership rejection and handle cleanup. This is bounded evidence. The shared binding at the verified initial system breakpoint passed one ordinary fresh resource SDK lifecycle: core connection, game/CLI/monitor exit and session report succeeded. This boundary precedes DLL initialization. Native HMR, repeated-start reliability, ordinary player delivery and unrelated startup faults remain separate gates.

## Stock console commands / Штатные консольные команды

RU/EN, 2026-10-06 follow-up: named ConsoleCommand export is verified after core HMR, including automatic source-watch rebuild/apply in one owned process with state counter7 retained. This supersedes the new-export-HMR-pending scope below. Independent final archive and ordinary player acceptance remainopen; the control uses the repaired developer sandbox.

RU/EN, earlier 2026-10-06 snapshot, before the export-HMR check above: the SDK exposed xkit game map through a restricted borrowed-core client. It retained the exact resident module through dispatch, did not stop payloads and refused to reconnect a stopped core. CLI live map loading and eight-hero reply preserved payload counter7/generation1. At that time, new-export HMR and player acceptance remained open. The later check above supersedes that HMR status.

RU: нативные локальные проверки отдельно проверяют допуск запроса: свой поток окна, форму запроса и границы текста. Это не разрешение выполнить команду: `DispatchConsoleCommand` дополнительно проверяет инструкции игры. Сгенерированный script dispatcher исполняется в тестовом процессе с настоящим хвостовым переходом, включая отключение callback. Игра для этих проверок не запускается.

EN: Local native checks isolate console request admission: window thread ownership, request shape and text bounds. Admission does not authorize execution; `DispatchConsoleCommand` additionally validates game instruction sites. Tests execute the generated script dispatcher with its actual tail jump and callback removal. These checks do not launch the game.

RU, 2026-10-06: экспериментальная привязка `h5::DispatchConsoleCommand` в `include/h5/console.hpp` использует штатные создание строки, разбор команды и освобождение памяти игры. Вызывающий сначала проверяет поддерживаемую сборку. Все три операции выполняются на одном подтверждённом потоке игрового окна: allocator игры использует TLS. Текст — непустая UTF-16 строка до 4095 символов в запросе фиксированного размера и версии. Неизвестные сигнатуры, поток, процесс или форма запроса отклоняются.

EN: The experimental binding uses the game's own wide-string constructor, inline console dispatcher and allocator release. The caller verifies the supported build first. All operations stay on one verified game-window thread because the allocator uses TLS. A fixed-size/version request carries nonempty bounded UTF-16 text. Unknown sites, thread/process and request shapes are rejected.

RU: `true` означает только возврат native dispatcher. Ошибка имени команды, отложенный переход и фактический результат проверяются отдельно. Живые контроли подтвердили возврат `help`, изменение зарегистрированной настройки, изображение перехода меню → WorkshopPolygon и отдельный ответ со списком восьми героев. Перед первым resume тестовая игра получила исправление порядка импортов и существующий контроллер. Это не приёмка обычной player-поставки, произвольных команд или обновления этой привязки через HMR. Прототип ещё не подключён к пользовательской CLI.

EN: True means only that the native dispatcher returned. Command recognition, deferred transitions and game effects need separate evidence. Owned controls confirmed help returning, a registered setting changing, a captured menu-to-WorkshopPolygon transition and a separate eight-hero reply. The sandbox used import repair and its existing controller before first resume. Ordinary player delivery, arbitrary command safety and HMR of this new binding are not certified; public CLI integration is pending.

## Owned image placement / Размещение собственной DLL

RU, 2026-10-05: при разборе обычного запуска подтверждены экспортируемые символы анимационной библиотеки `GrannyInt32Type`, `_GrannyReadEntireFileFromMemory@8`, `_GrannyGetFileInfo@4`, `_GrannyDataTypesAreEqual@8` и `_GrannyConvertTree@12`. Это каталог наблюдений, не новые вызываемые wrappers. Полный дамп показывает неверную вложенную ссылку в описании загруженной модели; соответствующие таблицы самой DLL перемещены правильно. Вызывающий код и момент записи неверной ссылки ещё не установлены. Общая правка указателей этим результатом не разрешается.

EN: These export names were verified against the pinned animation image and an owned full dump. The file-info conversion path compares a legacy model-side type tree against current library definitions. The stale reference belongs to the loaded file section, while checked mapped-library pointers relocate correctly. Typed layouts, writer provenance and a safe repair remain unverified. [Research record](https://xaaalera.github.io/heroes5-knowledge/reference/research-diary/).

RU: `VerifyOwnedImagePlacement` проверяет зависимость в остановленном собственном процессе. Требуются PID, время создания, путь игры и handle файла из события загрузки DLL. Проверяются сборка игры, файл/хеш зависимости, точная база, PE-заголовок и одна обязательная читаемая область данных. Неизвестная идентичность или неверное размещение отклоняются. Функция не продолжает событие, не завершает процесс и не закрывает переданный file handle; orchestration выполняет SDK.

EN: The caller must invoke this check while its owned process is suspended at `LOAD_DLL_DEBUG_EVENT`, before continuing dependency initialization. `ImagePlacementContract` describes a pinned x86 PE image and one required data range. The helper validates explicit ownership via OpenOwnedProcess, file identity/hash, allocation base/type, image header and readable committed data. It does not discover a process or change memory. SHA-256 hashes the retained loader file handle, not a reopened pathname, and preserves the caller's file cursor. The caller must retain a readable, stable file object during verification. Required data rejects execute-only or guarded protection and is read in bounded chunks with exact byte counts. Native tests cover replacement of a pathname while the original file remains open, empty-file hashing, cursor preservation and bounds/ownership negatives; loader integration and its positive failure cleanup remain separate acceptance gates. Current-game mapping checks do not retroactively prove this pre-use boundary.

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
| BankLayout: bank army selection | bank-reference InstallSelector | Existing native selector test and live bank-window research |
| PlacementRenderer: formation rendering | deployment-preview InstallRendererHook | Legacy compiled hook and retained live predictor research |
| AdventureAttack: entry before battle | deployment-preview InstallRendererHook | Legacy pre-battle hook |
| PlacementHover: pointing at a formation | deployment-preview InstallRendererHook | Legacy hover hook |
| ScriptDispatchCall: script command dispatch | devkit game_control/HMR | Live call replacement/rollback plus machine-state test |
| ExitRequest: queue an owned-game exit | devkit native mailbox and SDK bridge | Verified nullable context/register contract and owned native exit0 |

These facts are extracted from existing source; catalog extraction does not create new accuracy or live coverage. Full evidence history: [research diary](https://xaaalera.github.io/heroes5-knowledge/reference/research-diary/). Native package game files are never bundled.

### Shared script observers / Общие наблюдатели диспетчера

RU: `script_observers.hpp` позволяет нескольким SDK-плагинам наблюдать обработку игровых скриптов вместе с терминальными командами. Почтовый ящик команд владеет единственным перехватом; каждый плагин занимает отдельную подписку. При обновлении ядра старая подписка снимается до передачи состояния, новая добавляется после восстановления. Удаление одного плагина не отключает остальные.

EN: `FindScriptObservers` recognizes only the SDK-owned dispatcher contract: committed private RX/RW regions, supported descriptor version, exact observer prefix, original CALL destination and sealed code digest. `AddScriptObserver`/`RemoveScriptObserver` publish and remove one callback using Windows interlocked operations. Capacity is64 subscribers. Calling code validates the owned game identity/build first; this contract does not authenticate against other code already permitted to modify that process.

RU/EN: Registration, removal and invocation require the same validated game thread. Callbacks use `cdecl void()`, must return normally, cannot re-enter the runtime or retain payload/state pointers. The owner saves/restores registers, stack, flags and FPU/SSE; each callback enters with the direction flag clear. This is an observation boundary, not a guarantee that loading a map or arbitrary world changes are safe there. Unknown detours remain rejected. SHA-256 uses [Windows CNG BCryptHash](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcrypthash), supported on Windows10/11; link bcrypt through the library CMake target.

RU: без почтового ящика SDK создаёт нативный общий диспетчер на игровом потоке, только при исходной подписи. Он сохраняет исходный адрес возврата и аргументы стека. Код не зависит от DLL владельца и остаётся до выхода игры; при нуле подписчиков продолжает вызывать исходную функцию. Один экземпляр переиспользуется при HMR и отключении плагинов.
EN: `CreateScriptObserverStorage` prepares sealed resident code with a tail jump to the original dispatcher. The SDK publishes it after validating the original CALL; publication failure retains prepared storage if rollback/cache state is uncertain. Several ordinary player DLLs share this owner. Unloading a payload never frees the dispatcher or another plugin's callback.

`ExitRequest` identifies the existing game routine that queues exit with a null context. The SDK bridge checks its signature and game thread before calling it. Exit uses a separate registered window message and cannot reinterpret a payload function ID. The diagnostic client verifies ownership/build at startup, retains that kernel process handle, waits for actual exit and then skips remote cleanup against the terminated process.

## Limits
Windows x86 and the exact hashed Universe build only. Signatures identify sites, not arbitrary-safe detour recipes. No inferred function prototypes/structure offsets are exposed yet. Consumers must preserve registers, stack, FPU, thread affinity and callback lifetime. OpenOwnedProcess validates supplied provenance; the caller is responsible for owning the launch and closing the returned handle. It does not authorize attaching to an owner's unrelated game.

RU: расширять каталог по мере подтверждения. Typed wrappers и layouts добавлять только после доказанной сигнатуры, calling convention, границ lifetime и теста. Непроверенные гипотезы хранить в research, не выдавать за готовый API.

## Verification

### Camera input capture

`h5/camera_input.hpp` provides shared subscriptions before adventure-camera input dispatch. Subscribers vote to suppress mouse or keyboard camera input. The dispatcher preserves the original virtual target and existing suppression arguments. With no subscribers it forwards normally.

Subscription changes run on the validated game thread. Remove each core's callback before stopping or replacing that core. The dispatcher is self-contained resident code; it does not retain a stopped DLL callback. Its descriptor, full emitted code and digest are checked before reuse.

RU: библиотека позволяет консоли подавлять ввод камеры над своей панелью, сохраняя управление над картой. Она не блокирует все игровые интерфейсы и горячие клавиши: они обрабатываются раньше камеры. Нативная проверка покрывает разные виртуальные обработчики, объединение флагов, независимое удаление подписок, отсутствие подписчиков и сохранение состояния вычислений.

RU, 2026-10-06: проверен штатный базовый обработчик камеры поддерживаемой сборки. Конструктор связывает `camera_zoom_mouse` с группой мышиного ввода; при запрете мыши обработчик пропускает эту группу. Запрет клавиатуры управляет отдельной группой. Эмуляция исходного машинного кода прошла для всех четырёх сочетаний флагов; обработчик привязки команды в этом контроле заменён заглушкой. Проверка подтверждает ветвление и аргументы, но не движение камеры от настоящего колесика. Есть также путь передачи события другому контроллеру; его поведение этой проверкой не подтверждено.

EN: The pinned base-camera handler was checked against its constructor: `camera_zoom_mouse` belongs to the mouse binding group, skipped when mouse input is suppressed. Keyboard bindings have a separate gate. Original machine-code emulation passed all four flag combinations with a stubbed binding helper. This proves branch/argument behavior, not physical wheel movement or behavior delegated to another controller.

RU/EN: A Windows wheel message alone is insufficient evidence for a game's device-input path. Microsoft documents [WM_MOUSEWHEEL](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel) and the separate [DirectInput mouse state](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416630(v=vs.85)). This distinction does not establish which route the current game session uses.

Native test: known SHA vector, corrupt correctly-named H5_Game.exe, wrong process creation, wrong path with correct creation, failed-open handle cleanup, hook resume addresses and decoded CALL target. Consumers compile against the canonical headers. Existing source/DLL accuracy evidence remains tied to its original hashes; extracting constants does not transfer it to a rebuilt predictor.

RU: тест теперь доходит до проверки хеша правильно названного H5_Game.exe и пути процесса с правильным creation time; также сверяет отсутствие утечки handle при отказе. Проверка имени файла не подменяет проверку хеша.

Shared-observer checks: API native1/1 and SDK native4/4 PASS; Python68/68 covers mailbox arguments, two-observer machine state, independent removal and acceptance cleanup failures. Owned live sdk-shared-dispatch-20261005T010236/report.json passed two subscribers plus Lua reads, payload/core HMR, rejected-core rollback, independent removal/re-add, zero callbacks after stop and game exit0. Full38.330s, all five runtime-source hashes match; subsequent checker-only cleanup/report changes have separate unit coverage. No release-package or scene-rendering claim is made by that control.

Latest native-owner control sdk-shared-dispatch-20261005T014926/report.json PASS, full32.383s: no developer mailbox, two observers, payload/core HMR, rollback, independent remove/readd, zero subscriptions and game exit0. Native machine checks also prove stack arguments, GPR/flags/x87/SSE with a second subscriber that changes FPU state, and separation of high payload commands from exit. SDK native4/API1/Python69 PASS.

Ordinary same-source two-DLL release control sdk-native-owner-release-20261005T020619/report.json PASS,20.160s: callbacks were registered before any diagnostic controller, both ran, stopping alpha preserved beta, all slots then cleared, native exit0 and sandbox files restored. ProcDump monitored this run. The preceding015357 launch crashed before callback inspection; its cause remains unknown and its report is failed. Successful monitored startup does not establish a startup-crash fix. Visual scene capture was not part of these controls.

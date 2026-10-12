# Game API documentation / Документация Game API

## Purpose

RU: Единственный текущий справочник библиотеки находится на сайте.
EN: The website owns the library's living reference.

## Operation

## RU

- [Game API — русский справочник](https://xaaalera.github.io/heroes5-knowledge/reference/game-api/).

## EN

- [Game API — English reference](https://xaaalera.github.io/heroes5-knowledge/en/reference/game-api/).
- [SDK xkit](https://xaaalera.github.io/heroes5-knowledge/modding/devkit/).

## Limits

RU: Здесь нет параллельной инструкции. Численные привязки остаются в исходниках; прежние датированные проверки сохраняются в истории Git и архиве исследования.
EN: This file is a source-to-documentation pointer. Numeric bindings remain in code; prior dated verification records remain in Git history and the retained research archive.

## Verification

RU: Изменение контракта требует обновить каноническую статью и пройти независимые проверки её окончательного текста.
EN: Contract changes require updating the canonical article and independently checking its final text.

RU: Для изменений входов армии, исходной силы, завершённой расстановки, размеров боевого поля и резидентного хранения проверять раздел справочника «Что можно переиспользовать»: доступность версии, ограничения чтения и время жизни данных.
EN: For army inputs, source strength, committed deployment, battlefield dimensions and resident storage, check the reference's “Available building blocks” section for version availability, reading constraints and data lifetime.

RU: Для рабочего дополнения native_ui.hpp обновлять раздел справочника «Читать штатный игровой интерфейс»: состояние публикации, стабильность снимка, единицы геометрии и время жизни объекта. Создание виджетов и ввод проверять отдельно от чтения.
EN: For the working native_ui.hpp addition, maintain the reference section “Read the native game interface”: publication status, snapshot stability, geometry units and object lifetime. Verify widget creation and input separately from reading.

EN: The unpublished native UI extension now includes bounded UTF-16 text copies from WindowTextView, its copying ITextView setter, and native placement mutation. Document the 16,384-code-unit limit and that placement positions use the widget's existing alignment rather than resolved screen coordinates. Live read evidence and refusal/unit checks do not certify native mutation or the completed comparison window; retain separate acceptance receipts.

EN: The working scroll addition separates inherited children from IScrollableContainer items. Registration may rename and reparent engine-owned text; callers must rebind after payload replacement and let native reflow own text height. Positive registration does not certify wheel delivery, visible scrolling, modal keyboard behavior or the final player artifact.

RU: Неопубликованная диагностика различает обычных детей корня и список модальных окон боевого экрана; читает полный объект экрана отдельно от его интерфейса. Проверки списка ограничены структурой и не подтверждают работоспособность окна. Неудачный эксперимент создания/закрытия модального окна удалён; такого контракта библиотека сейчас не предоставляет.
EN: Unpublished diagnostics distinguish root children from the combat screen modal list and resolve the complete screen separately from its interface. List checks establish structure, not window health. The rejected modal construction/removal experiment was removed; the library currently exposes no such mutation contract.

RU: Рабочий SetNativeWindowModalInput включает штатный барьер неиспользованного ввода у существующего виджета. Вызывающая сторона выключает его при закрытии и смене поколения. Он не регистрирует модальное окно экрана и сам по себе не выбирает группу клавиш UI; физический Esc и колесо требуют отдельных проверок. Неиспользуемые черновики фокуса и активной цели удалены после отрицательных проверок.
EN: Working SetNativeWindowModalInput enables the stock unhandled-input barrier on an existing widget. The caller disables it on close and generation retirement. It neither registers a screen modal nor selects UI keybindings; physical Esc and wheel need separate acceptance. Unused focus/active-target drafts were removed after negative checks.

EN: Scroll text refresh now resets both slider position and content translation through the stock reset-to-top method. Retained live long-to-short refresh evidence accepts that route; wheel delivery remains independently unaccepted. Correct scan-code Escape is accepted on the ordinary child-window backend, so no modal construction contract is needed for this behavior.

EN: Working native cleanup now uses ReadNativeCombatRootPresence to distinguish retained roots, verified absence and failed reads. ReadNativeScrollTextWindow resolves the unique bounded text descendant before updating an existing scroll item. Update the canonical RU/EN reference and preserve uncertainty/identity regression coverage with these bindings.

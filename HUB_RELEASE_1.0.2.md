# GlassBoard 1.0.2 — поставка в PharosHub, 2026-10-20

Каноническая версия: `CMakeLists.txt`, `project(GlassBoard VERSION 1.0.2 ...)`. Windows-метаданные EXE генерируются из неё; runtime, заголовок окна, имя ZIP и Setup согласованы. Постоянный AppId: `{CBA09359-8F14-4A2D-8B13-6EF568933B14}`; идентификатор Хаба — `glass-board`, EXE — `GlassBoard.exe`.

## Артефакты

- Установщик: `Z:\CPP\GlassBoard\dist\GlassBoardSetup_1.0.2.exe`.
- Общая копия: `Z:\Yandex.Disk\PharosHub\installers\glass-board\GlassBoardSetup_1.0.2.exe`.
- SHA-256 установщика: `15D3BD380EFC829A902209B56FF65D3628CA80943EC76C7A798D2F1A8DEC6D3E`.
- Portable: `Z:\CPP\GlassBoard\dist\GlassBoard-1.0.2.zip`, рядом SHA-256.
- Manifest: `Z:\Yandex.Disk\PharosHub\releases\glass-board\manifest.json`: версия 1.0.2, 29 файлов, все SHA-256 совпали. Пакет включает app-local Qt/CRT и лицензии, не включает тестовые снимки, настройки и пользовательские данные.
- Новый установщик Хаба: `Z:\Yandex.Disk\PharosHub\installers\hub\PharosHubSetup_0.2.2.exe`.
- SHA-256 Хаба: `B652D18805DC2FF7F80759136F29662F28AC8C6D4D7FC8E7C824DD765FE93B80`.
- Семейный ZIP Хаба: `Z:\Yandex.Disk\PharosHub\hub\PharosHub-0.2.2-windows-x64.zip`; SHA-256 `645EC404A0768DB46B26A5DA0C061BD782323AEF125345A6CE61BC9B5228AE09`. Проверены все 1300 файлов архива; каталог содержит восемь записей с пустыми путями EXE.

## Проверки

1. GlassBoard: Release-сборка, запуск с системным PATH в отдельном INI-профиле, рисование, стирание, undo/redo, resize и захват окна passed. Smoke пишет только в отдельный каталог доказательств; проверены неизменность состава и SHA-256 программного пакета.
2. Настоящий установщик GlassBoard собран Inno Setup. Эквивалентные тестовые установщики с отдельным AppId: установка предыдущего portable 1.0.1 → обновление до 1.0.2 → запуск установленной версии без Qt в PATH → удаление. Версия Setup/EXE/реестра/окна согласована для 1.0.2; внешний пользовательский маркер сохранён. У прежнего EXE 1.0.1 не было ProductVersion — исходная версия определена по существующему пакету 1.0.1. Продуктовый AppId и настоящий профиль не изменялись.
3. Хаб установил опубликованный GlassBoard своим методом `LauncherModel::installRelease` в `qttest/PharosHubInstallerTests`. Установленная копия прошла ту же проверку с Windows-плагином и системным PATH. Ошибочный пробный запуск с унаследованным offscreen-плагином не засчитан; проверочный скрипт теперь очищает Qt/QML-переменные окружения.
4. Хаб: install-safety и hub-ui CTest — 2/2 passed. Эквивалентные установщики 0.2.1 → 0.2.2 с отдельным AppId прошли install/upgrade/startup/version/uninstall с сохранением внешнего маркера. Промежуточно проверен переход 0.2.0 → 0.2.1. В 0.2.2 `hubSettings()` явно учитывает выбранный формат, поэтому `--data-dir` действительно изолирует INI от обычных настроек пользователя.
5. Окно семейного Хаба 0.2.2 просмотрено: GlassBoard первым в списке, версия 1.0.2 и активная кнопка «Установить» при подключённой общей папке. Семейный каталог не содержит абсолютных путей разработчика.

Доказательства:

- GlassBoard lifecycle: `Z:\CPP\GlassBoard\build\installer-lifecycle-040e83ab846048f5a0117a80c1a39577\report.json`.
- Публикация: `Z:\CPP\GlassBoard\build\hub-publication.log`; установка Хабом: `build/hub-installation.txt`.
- Проверка установленной Хабом копии: `Z:\CPP\GlassBoard\build\package-smoke-125ec72adc264d9c8fe36fbc93bc9e73`.
- Хаб lifecycle: `Z:\CPP\PharosHub\build\installer-lifecycle-fff0474273c54434a3f69a60bd448562\report.json`.
- Семейный архив: `Z:\CPP\PharosHub\build\glassboard-package-0.2.2.log`.
- Семейная плитка и снимок: `Z:\CPP\PharosHub\build\glassboard-cloud-ui-113fb95db7b24bd499b434810c0d39e4\output`.

Правила дальнейшей разработки: `AGENTS.md`. Сборка: `installer/build-installer.ps1`; проверка: `verify_package.ps1`; публикация: `publish-hub.ps1 -LifecycleReport <report.json>`. Старые артефакты, включая промежуточный Хаб 0.2.1, сохранены; для распространения выбирайте 0.2.2. Содержимое опубликованных программных пакетов не менялось после публикации. Последующие правки README и проверочных скриптов сохранены в исходниках.

Проверена локальная папка Яндекс.Диска. Подтверждения завершения серверной синхронизации нет.

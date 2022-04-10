1. Запустить экшон SP Release https://github.com/skyrim-multiplayer/skymp/actions/workflows/sp-release.yml. Он создаст пулл реквест, обычно это занимает минуту.
2. Открыть созданный пулл реквест и проверить грамматику, стиль, а также логику пулл реквеста. Он должен собирать несколько текстовых файлов в один и менять везде цифру версии.
3. Принять пулл реквест. Ничего не боимся! В случае всего всё очень легко откатывается.
4. Запустить экшон SP Types Update https://github.com/skyrim-multiplayer/skymp/actions/workflows/trigger-sp-types-update.yml. Он обычно выполняется не более минуты.
5. Убедиться, что сюда прилетел коммит в main ветку https://github.com/skyrim-platform/skyrim-platform.
6. Ждать, пока соберётся PR Windows, PR Windows AE экшоны в мейн ветке.

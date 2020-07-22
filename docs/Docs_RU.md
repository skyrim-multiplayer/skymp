

### Papyrus-типы из оригинальной игры
* Все типы в SkyrimPlatform имеют такое же название как и в Papyrus, например: `Game`, `Actor`, `Form`, `Spell`, `Perk` и т.д.

* Для использования типов из Papyrus, включая вызов методов и статических функций, которыми они обладают, их нужно импортировать:
```typescript
import { Game, Actor } from  "../skyrimPlatform"
```
### Нативные функции
* У большинства типов присутствует перечень нативных функций, они делятся на статические функции (`Native Global` в Papyrus) и методы (`Native`).

*   Статические функции вызываются на типе:
```typescript
let sunX = Game.GetSunPositionX();
let pl = Game.GetPlayer();
Game.forceFirstPerson();
```

*   Методы вызываются на объекте:
 ```typescript
 let isPlayerInCombat = pl.isInCombat();
 ```

* Список типов оригинальной игры с документацией можно найти здесь: https://www.creationkit.com/index.php?title=Category:Script_Objects
* Вызов функций из оригинальной игры доступен только внутри обработчика события `update` (см. ниже). При попытке сделать это в другом контексте, исключение будет выброшено.

### Form
* Форму (`Form`) наследуют большинство игровых типов, имеющих методы, такие как `Actor`, `Weapon` и т.д.
* У каждой формы есть ID, представляющий собой 32-битное беззнаковое число (`uint32_t`). В SkyrimPlatform представлена типом `number`.
* Если вам нужно найти форму по ее ID, используйте `Game.getFormEx`.  Обратите внимание, что именно `Game.getFormEx`, а не `Game.getForm`. Последняя всегда возвращает `null` для ID выше 0x80000000 (поведение оригинальной игры).
* Получить ID формы можно с помощью метода `getFormID`. Гарантируется, что `Game.getFormEx` будет находить форму по ID, который был возвращён данным методом, если форма не была уничтожена игрой.

### Безопасное использование объектов
* После того как вы получили объект нужно убедиться, что он не является `null`:
```typescript
let actor = Game.findClosestActor(x, y, z, radius);
if (actor) {
	let isInCombat = actor.isInCombat();
}
```
Или
```typescript
let actor = Game.findClosestActor(x, y, z, radius);
if (!actor) return;
let isInCombat = actor.isInCombat();
```
* Гарантируется, что `Game.getPlayer` никогда не возвращает `null`.

### Необработанные исключения
* Необработанные JS-исключения будут выведены в консоль вместе со стеком вызовов.
* Необработанные Promise rejections также выводятся в консоль.
* Не выпускайте плагины, в которых есть известные вам ошибки, которые не обрабатываются. Стабильная работа SkyrimPlatform не гарантируется при необработанных исключениях.


### Сравнение объектов
* Для сравнения объектов в SkyrimPlatform вам нужно сравнить их ID:
```typescript
if (object1.getFormId() === object2.getFormId())
```

### Приведение объектов к строке
* Типы, портированные из Papyrus имеют ограниченную поддержку ряда операций, нормальных для обычных JS-объектов, таких как `toString`, `toJSON`.
```typescript
Game.getPlayer().toString(); // '[object Actor]'
JSON.stringify(Game.getPlayer()); // `{}`
```

### Приведение типов
* Если у вас есть объект `Form`  являющийся оружием, а вам нужен `Weapon`, вы можете использовать приведение типов:
```typescript
let sword = Game.getFormEx(swordId); // Получаем Form
let weap = Weapon.from(sword); // Приводим к Weapon
```
* Если вы указали  ID формы которая на самом деле не является оружием переменная `weap` будет равна `null`.
* Передача `null` в функцию для приведения типов в качестве аргумента не бросит исключение , а вернет `null`:
```typescript
ObjectReference.from(null); // null
```
* Попытка привести к типу, не имеющему экземпляров или несовместимому по иерархии наследования, также вернёт `null`:
```typescript
Game.from(Game.getPlayer()); // null
Spell.from(Game.getPlayer()); // null
```
* Также можно использовать приведение типов для того чтобы получить объект базового типа, включая `Form`:
```typescript
let refr = ObjectReference.from(Game.getPlayer());
let form = Form.from(refr);
```
* Приведение объекта к его же типу вернёт оригинальный объект:
```typescript
let actor = Actor.from(Game.getPlayer());
```

### Papyrus-типы, добавленные SkyrimPlatform
* SkyrimPlatform в настоящее время добавляет только один тип: `TESModPlatform`. Экземпляры данного типа не существуют по аналогии с `Game`. Ниже перечислены его стататические функции.
* `moveRefrToPosition` - телепортирует объект в заданную локацию и позицию.
* `setWeaponDrawnMode` - заставляет актёра всегда держать оружие достанным/убранным.
* `getNthVtableElement` - получает сдвиг функции из виртуальной таблицы (для реверс-инжиниринга).
* `getSkinColor` - получает цвет кожи ActorBase.
* `createNpc` - создаёт новую форму типа ActorBase.
* `setNpcSex` - изменяет пол ActorBase.
* `setNpcRace` - изменяет расу ActorBase.
* `setNpcSkinColor` - изменяет цвет кожи ActorBase.
* `setNpcHairColor` - изменяет цвет волос ActorBase.
* `resizeHeadpartsArray` - изменяет размер массива частей головы ActorBase.
* `resizeTintsArray` - изменяет размер массива TintMasks главного героя.
* `setFormIdUnsafe` - изменяет ID формы. Небезопасно, используйте на свой страх и риск.
* `clearTintMasks` - удалить TintMasks для данного Actor или ГГ, если Actor не передан.
* `pushTintMask` - добавить TintMask с опр. параметрами для данного Actor или ГГ, если Actor не передан.


### Асинхронность
* Выполнение некоторых игровых функций занимает время и происходит в фоновом режиме. Такие функции в SkyrimPlatform возвращают `Promise`:
```typescript
Game.getPlayer().setPosition(0,0,0).then(() => {
	printConsole('Teleported to the center of the world');
});
```
```typescript
Utility.wait(1).then(() => printConsole('1 second passed'));
```
* При асинхронном вызове выполнение продолжается немедленно:
```typescript
Utility.wait(1);
printConsole(`Выведется сразу, а не через секунду`);
printConsole(`Надо было юзать then`);
```
* Вы можете использовать `async`/`await`, чтобы код выглядел синхронным:
```typescript
let f = async () => {
	await Utility.wait(1);
	printConsole('1 second passed');
};
```

### События

* На данный момент в SkyrimPlatform есть возможность подписаться на собственные события: `update`  и `tick`.

* `update` - это событие, которое вызывается один раз за каждый кадр в игре (60 раз в секунду при 60 FPS) после того как вы загрузили сохранение или начали новую игру.
```typescript
import { on } from  "../skyrimPlatform"
on('update', () => {
// На данном этапе методы всех импортированых
// типов уже доступны.
});
```

* `tick` - это событие которые вызывается один раз за каждый кадр в игре сразу после запуска игры.
```typescript
import { on } from  "../skyrimPlatform"
on('tick', () => {
// Тут нет доступа к игровым методам.
});
```
* А также на игровые события, такие как `effectStart`, `effectFinish`, `magicEffectApply`, `equip`, `unequip`, `hit`, `containerChanged`, `deathStart`, `deathEnd`,  `loadGame`, `combatState`, `reset`, `scriptInit`, `trackedStats`, `uniqueIdChange`, `switchRaceComplete`, `cellFullyLoaded`, `grabRelease`, `lockChanged`, `moveAttachDetach`, `objectLoaded`, `waitStop`, `activate`.

* С помощью `on` можно навсегда подписаться на событие.
```typescript
import { on } from  "../skyrimPlatform"
on("equip", (even) => {
	printConsole(`actor: ${even.actor.getBaseObject().getName()}`);
	printConsole(`object: ${even.baseObj.getName()}`);
});
```
* С помощью `once` можно добавить обработчик, который вызовется единожды при следующем срабатывании события.
```typescript
import { once } from  "../skyrimPlatform"
once("equip", (even) => {
	printConsole(`actor: ${even.actor.getBaseObject().getName()}`);
	printConsole(`object: ${even.baseObj.getName()}`);
});
```
* В переменной `even` всегда содержатся переменные касающиеся того события на которое вы подписаны.

### Хуки
* Хуки позволяют перехватывать запуск и завершение некоторых функций движка игры.
* Поддерживаемые хуки в данный момент: `sendAnimationEvent`
```typescript
import { hooks, printConsole } from  "../skyrimPlatform"
hooks.sendAnimationEvent.add({
	enter(ctx) {
		printConsole(ctx.animEventName);
	},
	leave(ctx) {
		if (ctx.animationSucceeded) printConsole(ctx.selfId);
	};
});
```
* `enter` вызывается перед запуском функции. `ctx` содержит аргументы, переданные в функцию, а также `storage` (см. ниже).
* `leave` вызывается перед завершением функции. `ctx` содержит возвращаемое значение функции, помимо того, что в нём было после завершения `enter`.
* `ctx` - это один и тот же объект для вызовов `enter` и `leave`.
* `ctx.storage` служит для хранения данных между вызовами `enter` и `leave`.
* Скриптовые функции недоступны внутри обработчиков `enter` и `leave`.

### Собственные методы и свойства SkyrimPlatform
* Существуют методы, например, такие как `printConsole()`, которые можно вызвать сразу после импортирования. Они не относятся ни к одному из игровых типов.
* `printConsole(...arguments: any[]): void` - вывод в игровую консоль, открывающуюся на клавишу `~`.
```typescript
import { printConsole, Game } from  "../skyrimPlatform"
on('update', () => {
	printConsole(`player id = ${Game.getPlayer().getFormID()}`);
});
```
* `worldPointToScreenPoint` - преобразовать массив точек игрового мира в массив точек на экране пользователя. Точка на экране обозначается 3 числами от -1 до 1.
* `on(eventName: string, callback: any): void` - подписаться на событие с именем `eventName`.
* `callNative(className: string, functionName: string, self?: object, ...args: any): any` - вызвать функцию из оригинальной игры по имени.
* `getJsMemoryUsage(): number` - получить количество оперативной памяти, используемой встроенным JS-движком, в байтах.
* `storage` - объект, служащий для сохранения данных между перезагрузкой скриптов.
* `settings` - объект, предоставляющий доступ к настройкам плагинов:
```typescript
import { settings, printConsole } from  "../skyrimPlatform"
let option = settings["plugin-name"]["my-option"];
printConsole(option);
```
Файл с настройками для плагина называется `имя-плагина-settings.txt` и должен находиться в папке `Data/Platform/Plugins`.
Формат файла - JSON, расширение `.txt` - для удобства пользователей.

### Изменение игровых консольных команд
* SkyrimPlatform позволяет изменить реализацию любой консольной команды игры, для подобной модификации вам необходимо получить объект консольной команды, передав имя команды  в метод `findConsoleCommand(commandName)` короткое или длинное.
```typescript
let getAV = findConsoleCommand("GetActorValueInfo");

let getAV = findConsoleCommand("GetAVInfo");
```
* Получив такой объект вы можете изменить короткое (`shortName`) или же длинное (`longName`) имя команды, а также количество принимаемых аргументов (`numArgs`) и функцию (`execute`) которая будет выполнятся при вызове этой консольной команды через консоль игры.
```typescript
getAV.longName = "printArg";
getAV.shortName = "";

getAV.execute = (refrId: number, arg: string) => {
    printConsole(arg);
    return false;
};
```
* Возвращаемое значение вашей новой реализации указывает на то, будет ли выполнена оригинальная функция этой команды.
* Первым аргументом передаётся FormId объекта, на котором вызывается консольная команда или 0 в случае его отсутствия.
* Остальными параметрами будут аргументы, с которыми была вызвана консольная команда, имеющие тип `string` или `number`.
* Поскольку игровые функции не доступны в данном контексте, вы должны зарегистрировать обработчик события `update` с помощью `once`, если требуется вызвать игровую функцию при вызове консольной команды:

```typescript
getAV.longName = "ShowMessageBox";
getAV.shortName = "";

getAV.execute = (refrId: number, arg: string) => {
    once("update", () => {
        Debug.messageBox(arg);
    });
    return false;
};
```

### HTTP-запросы (experimental)
SkyrimPlatform предоставляет ограниченную поддержку HTTP-запросов.
В данный момент только `get` доступен.
```typescript
import { HttpClient, } from  "../skyrimPlatfosrm";
let http = new HttpClient('vk.com', 80);
http.get('/').then(response => printConsole(response.body));
```
* В случае, если запрос завершился неудачно, `response.body` будет пуст.

### Hot Reload
* Hot Reload для SkyrimPlatform-плагинов поддерживается. Изменение содержимого `Data/Platform/Plugins` вызывает перезагрузку всех плагинов без перезапуска игры.
* Для полноценного использования это фичи, т.е. перезагрузки вашего плагина при Ctrl+S, возьмите за основу пример плагина https://github.com/skyrim-multiplayer/skyrimplatform-plugin-example
* При перезагрузке плагинов добавленные обработчики событий и хуков удаляются, прерываются асинхронные операции и обнуляются все переменные, кроме `storage` и его свойств.

### DumpFunctions
* В SkyrimPlatform встроен функционал, позволяющий вывести в файл `Data/Platform/Output/DumpFunctions.txt` информацию об игровых функциях (сочетание клавиш 9+O+L). Игра приостанавливается на несколько секунд в ходе работы DumpFunctions.

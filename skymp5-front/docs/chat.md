# Backend to Frontend

## UPDATE_CHAT_SHOW
### Скрыть/показать чат
**data:** true/false

## ADD_CHAT_MSG
### Добавить сообщение в чат
**data:** "Генерал тулий: #{ff0000}hello"
> Текст после #{rrggbb} примет заданный цвет

## UPDATE_CHAT_SHOWINPUT
### Изменить режим отображения поля ввода
**data:** 'true'/'false'/'auto'
> **'true'** - всегда показывать[chat.md](chat.md)
> **'false'** - всегда скрывать
> **'auto'** - от[server-settings.json](..%2F..%2Fbuild%2Fdist%2Fserver%2Fserver-settings.json)крывать при нажатии **f6**


#  Frontend to Backend
[server-settings.json](..%2F..%2Fbuild%2Fdist%2Fserver%2Fserver-settings.json)
## 'cef::chat:send'
### Игрок отправил какое то сообщение
**data:** 'Some text'

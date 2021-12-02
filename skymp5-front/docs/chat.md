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
> **'true'** - всегда показывать
> **'false'** - всегда скрывать
> **'auto'** - открывать при нажатии **f6**


#  Frontend to Backend

## 'cef::chat:send'
### Игрок отправил какое то сообщение
**data:** 'Some text'

To build the project, you will need a ts file collector. I use for example parcel. You can use any other one.
First install Parcel using npm:

```
npm install parcel-bundler -D
```

To install all necessary dependencies run

```
npm install
```

To build, run the command

```
npm run build
```

To make the build happen every time the files are changed run the command

```
npm run serve
```

Если не устанавливается parcel-bundler попробуйте очистить кэш (На свой страх и риск)

```
npm cache clean --force
```

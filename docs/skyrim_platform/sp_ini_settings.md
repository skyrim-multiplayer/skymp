## Skyrim platform settings
You can utilize custom settings of the Skyrim platform in your code to retrieve and modify values located at ```"Data/SKSE/Plugins/SkyrimPlatform.ini"```

### Current  settings
- ```LogLevel = 2``` - This parameter controls the level of Logging 
    >0 - trace; 1 - debug; 2 - info; 3 - warn; 4 - error; 5 - critical; 6 - none
- ```Cmd = false``` - This parameter controls whether the console is enabled.

- ```CmdOffsetLeft = 0``` - This parameter controls the console window offset from the left edge of the screen.

- ```CmdOffsetTop = 720``` - This parameter controls the console window offset from the top edge of the screen.

- ```CmdWidth = 1900``` - This parameter controls the width of the console window.

- ```CmdHeight = 317``` - This parameter controls the height of the console window.
> The default values are for full hd

### Add new parametrs
To create your standart value upon game startup, you need to locate the method Settings::GetPlatformSettings() and use there:
- ```SetInteger(section, key, value, comment)```
- ```SetFloat(section, key, value, comment)```
- ```SetBool(section, key, value, comment)```
- ```SetString(section, key, value, comment)```

You can also get values from file fields
- ```Int GetInteger(section, key, defaultValue)```
- ```Float GetFloat(section, key, defaultValue)```
- ```Bool GetBool(section, key, defaultValue)```
- ```String GetString(section, key, defaultValue)```

### Example
```c++
auto settings = Settings::GetPlatformSettings();

settings->SetInteger("Debug", "CmdHeight", 317, "; Set height for cmd");
settings->GetInteger("Debug", "LogLevel", spdlog::level::level_enum::info);
```

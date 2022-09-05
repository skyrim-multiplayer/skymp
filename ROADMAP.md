# Roadmap

Welcome to the roadmap of the project! You can see synchronizations here, but there probably would also be non-sync and even non-code parts in the future.

# Synchronization

| Feature | Status/TODO |
| --- | --- |
| **DONE:** | **Fully implemented and well-tested. Near to ideal sync.** |
| *Attributes - Health, Magicka, Stamina* | Done |
| *Death* | Done |
| *Inventory* | Done |
| *Craft (forge like)* | Done |
| **PARTIALLY:** | **Implemented features we plan to improve or fix.** |
| *Appearance* | https://github.com/skyrim-multiplayer/skymp/issues/929 |
| *Movement* | Add lag compensation; work on the details - walking speed and stuff, ask for help in ST. Send Movement to the server when the character has moved instead of still once every X ms. |
| *Character creation* | Add server-side validation |
| *Animation events* | Remove local delays when animation events are queued to the next frames. Disable collision when using furniture like chairs. Ask for help at ST. |
| *Damage* | Remove incorrect range check for shooting. Blood spatter and sounds must be in sync. |
| *Containers* | Incorrect “empty” label on a container. You should animate the opening of the container for everyone, including sounds. We probably should prepare a new API in the platform for serializing/deserializing the contents of the container, as is done in ST. This API may be the basis for future upgrades to this sync and inventory sync. 2 or more players should be able to loot a container at the same time. |
| *Equipment* | What is equipped in the hands is not saved when you restart the server, but it should. |
| *Picking up items* | Take into account ExtraData (charm, poison, etc., impossible without throwing away the corresponding items). The message about adding an item should be like in the original: “Added…”. Synchronize pickup sound. When activating a book item, the reading interface should open, not picking up a book in inventory. |
| *Picking up ingredients* | The message must be as in the original, the sound must be as in the original and synchronized, the object must not blink and/or move. |
| *Torches* | Player should be able to set a player on fire with a torch. It should be synced. Also we should remove torch from the inventory after some time on the server. In the current version, torches are limitless like arrows also are. |
| *Scripts* | Variables need to be preserved, needs to be figured out how events work differently from the original Skyrim and documented (at least). |
| *Console commands* | Make a list of popular console commands, implement them. They should be typed as in the original game. Their output should also be the same. Implement a system of permissions that determines who can use which commands. Perhaps make it possible to use them on the server from stdin (https://github.com/lionkor/commandline), as well as selectors like in Minecraft - applying to all entities or to specific ones. On the client, the player IDs in the console should show the server ID, not the local one. |
| **TODO:** | **Planned features. It will be an amazing journey!** |
| *Anims triggered by enemy* | Usually in PvP. Stagger, random dodge animation, sprint collision reaction. |
| *Picking up leveled items* | In CK they start with the word Dummy, eg. RandomPotion. I'm not sure how the server handles them now. |
| *Effects learning* | When eating an ingredient, the character learns its effects, this information should not be reset. |
| *Favorites* | When adding an item, spell, etc. to your favorites, this should be saved when you reconnect. |
| *Markers* | Locations that have been visited must be saved on the server. |
| *Dropping items* | First, without physics: spawn a discarded object in the legs. After implementing the physics, dropping items should work like it did in the original game. |
| *Physics* | Ability to kick objects, movable statics and even some containers (the blue palace has a physical container). To do this, implement a host switching mechanism. |
| *Dragging items* | In the original game you are able to drag items. |
| *Dungeon support* | Traps, puzzles and more - test, fix what doesn't work. Everything should work as in original game. State must be saved on the server. |
| *Alchemy* | Ability to create potions. |
| *Enchantment* | Ability to enchant and rename items. Enchanting requires a soul stone filled by the player, or a soul stone from the editor - without ExtraData. |
| *Leaning enchantments* | Learned enchantments should be saved when re-entering. |
| *Charge* | The ability to charge items with soul gems if the spell runs out, including staves (see Staffs). Maintain state of charge. |
| *Temper* | The ability to improve items works similarly to crafting. |
| *Lockpicking* | The ability to break open doors and chests, their lock level must be from the game files. Possibility to open with keys. Bonus: the required position of the lockpick should not change during hacking attempts, synchronize the sounds of hacking. |
| *Skills* | Disable the original leveling formulas and add the ability to level skills for existing synchronizations, such as combat skills. When developing each new synchra, take into account the associated skill. |
| *Perks and level up* | Attribute boost must work and persist (health, magicka or stamina). Selected perks must be saved. Then gradually make the existing perks work. |
| *Blocking* | The ability to repel an attack may require "Reverse Animation Events". |
| *Marksman* | Perhaps use a mod that allows you to look up and down to synchronize the vertical angle of the character. The arrow must have the correct initial speed, fly out from where it is needed. Perhaps it will be possible to pick it up - at the initial stage, you can not do it. Perhaps the arrows stuck in the characters should be synchronized. The server must remove 1 arrow after each shot. |
| *Pickpocketing* | |
| *NPC* | Make sure that the host change works, as well as the existing synchronizations in relation to NPCs - combat, activation of objects, equipment, etc. Support rendering of NPC templates on the server and applying its results on the client. NPCs should spawn normally (no bugs, inappropriate behavior, they should behave according to packages). Going forward, they should be able to travel out of sight of the players. |
| *Loot* | Ability to loot dead NPCs and players. |
| *Corpse physics and corpse dragging* | |
| *Dialogs* | Random remarks of bandits and other NPCs, dialogues with quest characters. Probably not the priority since ST exists. |
| *Horses* | We must always host the horse we ride. Combat should also be available. Mounted shooting may be introduced separately. |
| *Quests* | Learn how to serialize / deserialize the quest state of the game, or just draw the right words in the interface. Probably not the priority since ST exists. |
| *Dragons* | Dragons should be able to fly, attack, cast spells and shouts. The ability to capture dragon souls. Ask ST for help. |
| *Shouts* | Ability to equip and cast shouts. Learning words of power. Possibility to limit the study of words of power, i.e. determine who is a Dovakin and who is not. Iteratively test and add more and more shouts. |
| *Magic effects* | The server must be able to cast a magical effect on a person. The use of the potion should impose a magical effect, and not instantly change health parameters, as it happens now. The appearance of the magic effect is accompanied by the playback of the shader effect. Even if the moment of applying the magic effect on another player was not visible, when you enter the visibility zone, you should see the effect of the shader. The most popular effect archetypes should be implemented (with which we attack and heal). |
| *Spells* | TODO: Add a few words on spells sync |
| *Weather* | Learn how weather changes work in the original game and do the same. |
| *Time* | Implement global variables (GlobalVariable) on the server that affect time. Built-in ability to set the time as in def. time zone. Now we have UTC time like everything and rolls for sync, but Sekunda (moon) blinks. To avoid this, you should try not to set the time all the time, but change the timeScale so that the sky moves smoothly to the desired time point. |
| *Wait* | Players should be able to sleep/wait if all players on the server are willing to do so. |
| *Statics* | The server should “see” them (do everything lazily and once again not spoil the database. It is desirable to be able to reset the state of such an object, completely clearing everything). We need to be able to move them around with SetPosition, etc. |
| *ActorValues* | Now only stamina, mana and health, but you need to be able to manipulate all ActorValues on the server side. |
| *Sounds and music* | Music can definitely be synchronized within cells, work with combat music - now it is included in PvP. Now, also after death, the music of death is superimposed on another. We should be able to play sounds from Art. server in a specific location (Seems to be decided by SpSnippet). Also, you need to define a set of actions for which the sounds will be synchronized. |
| *Lipsync* | TODO: define the boundaries of this sync. |
| *Linked Refs* | As far as I remember, linking propagates activation - this is important for dungeons, there is often a lever linked to a door, for example. |
| *Staff crafting* | |
| *Playable monster races* | Replacing a player with any NPC (animals, monsters, etc.) |
| *Lycanthropy* | |
| *Vampirism* | Perks, extras, vampire spells from DLC. I’ll add on my own that I don’t see the meaning of the word in everything at all :) |

# Build system

These deps should be installed via vcpkg:
- ctpl
- frida
- tilted core
- tilted hooks
- tilted reverse
- tilted ui
- makeid
- cmrc
- chakracore (official port instead of overlay or just update overlay)

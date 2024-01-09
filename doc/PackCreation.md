# Poptracker pack creation for novices
As typed in a casual, explanatory fashion by a (former) confused bird
## **Introduction**
Hello! If you’re reading this, you probably found a game you really like and thought “I want to make a tracker for that!” If you’re like me, you probably also thought “How hard can it be?” followed by staring at a lot of other people’s packs, making confused changes, and copying everything - followed by more confusion when it doesn’t just work.  

That’s totally fine! This is all fairly complex stuff, with a few simple rules and a LOT of documentation that’s not really designed for your standard mk.1 human to understand on the first read-through. The programmers know it, I know it, and now you do too. So, what this guide is intended to do is show you the bare minimum to get things off the ground, and do so in a (hopefully humorous) fashion that helps mitigate the sheer number of things you have to learn from scratch. The actual documentation is great! But there’s just a lot to try to parse through all at once, especially when you don’t have context for what you're reading.  

Know what you’re getting into here first, though. If you’re doing this on your own, that means that you have to be able to understand the game’s logic, do some basic graphical editing, translate the game logic into a complete list of locations, and debug it all when it refuses to cooperate anyways. THIS WILL TAKE A WHILE.  Depending on how much time you can devote to it, expecting it to take a week or a month is not unreasonable. Between initial revisions, finding better ways to do what I was doing manually, and testing different layout presentations to find one that displayed cleanly, I’m pretty sure that I’ve spent at least 15 hours working on my first tracker. Some of that can be chalked up to a learning experience, but the vast majority comes down to this: coding takes time. After it’s done, you’ll also be expected to provide support for the pack too. If that is not what you are able to do, then this might not be for you – and that’s okay.

### **If you’re already somewhat familiar with JSON and Lua:**
	You’re probably going to find several parts of this guide needlessly simplistic, but them’s the breaks. There isn’t really a guide beyond “Find a pack that does what you want and copy bits until it works”. If you want to write a more advanced guide, go for it! Make a better guide than I did, and let us know about it so we can all do better.

### **Prepping our tools**
Before you do anything else, you’re going to want to do a few things to set up your workspace. There are tons of great options for doing so, but this one was A) recommended to me by someone else B) did not require a ton of effort, and C) worked great for me. 

- Visual Studio Code is a handy code editor that is a lot more flexible than Notepad++, and includes the ability to handle linting the JSON files we’re creating. In programming terms, linting is the equivalent of spell-check – it’s the ability for your editor to figure out where you forgot to put a comma, close a brace, or just tell you that what you’re typing doesn’t go there.  
The download link for VSCode is currently https://code.visualstudio.com/download  

 > Note: I use Windows. VSCode is cross-platform for all major OS flavors, but I don’t have any capacity to make sure everything’s the same on MacOS or Linux. 
  
 * Download and install VSCode. There are a few things we’re going to tweak and add once it’s ready to go. 
 * Next, set up a Github repository for your project. This will allow you both to have a location for the pack for people to download later, and will help you organize the changes you make to your files. To use Git properly, you’re going to want to use more than just the web interface. I like Github Desktop for keeping things simple, and it integrates nicely with VSCode. 
> I can’t speak to best practices using Git, but that’s my own inexperience plus only having me working on my own projects so far. If someone wants to contribute to this section, that would be fantastic.

 * Once you have a local folder set up for your repository, there’s a few things we’ll need. First, go ahead and grab the entire template pack from https://github.com/Cyb3RGER/template_pack
     * Note: This may be added to that template pack in the future. If you already have a local copy, no need to download it again :)
 * Once you’ve got that, go ahead and put it in that Git repository folder. On my system, the default location was Documents\GitHub\*project name* 
 * Finally, let’s double back to VSCode now that we have the bare bones in place for getting started. Remember how I mentioned Linting earlier? That’s actually not something that just works out of the box, because there’s no universal correct way to use JSON files.  
 There's a pack checker tool, available on GitHub at https://github.com/PopTracker/pack-checker. The pack checker tool is more accurate than the schema files, but it can't spellcheck you while you work. Use it before you finalize things!
 * The magical, mystical files that show VSCode how to fix our errors are called Schema. They’re pretty great! As of Poptracker version 0.21, we now have our own Schema files. They're available here https://poptracker.github.io/schema/packs/
     * We’re going to need to add them to VSCode, and we can actually just point to the web version in our settings. Go into your VSCode, the menu we need is in File -> Preferences -> Settings. On Mac / Linux it’s going to be in a slightly different spot, but it should be easy to find. In the settings menu, search for Schema. The option you need is “JSON: Schemas” and it should have an Edit In Settings button. Click on that. 
    * Depending on how many settings you’ve changed, there might not be much of anything here. What we’re going to do is associate those schema files we downloaded with the specific JSON files in our template. The lines you'll need to add are as follows:
``` 
"json.schemas": [  
    {
        "fileMatch": [
            "*/manifest.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/manifest.json"
    },
    {
        "fileMatch": [
            "*/items/*.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/items.json"
    },
    {
        "fileMatch": [
            "*/locations/*.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/locations.json"
    },
    {
        "fileMatch": [
            "*/layouts/*.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/layouts.json"
    },
    {
        "fileMatch": [
            "*/maps/*.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/maps.json"
    },
    {
        "fileMatch": [
            "*/settings.json"
        ],
        "url": "https://poptracker.github.io/schema/packs/strict/settings.json"
    }
]
```


>**NOTES FOR PEOPLE WHO HAVE NO EXPERIENCE WITH JSON:**  
 There’s a few things going on here that you need to be aware of, especially if you’re not doing things exactly like the examples describe. The * character is a wildcard, which I’m using to match anything ending in the expected name. So long as the names include the text in “filematch”, then it’ll apply that schema to the file. The “url” is a web URL (though it could be a local path if you wanted it set up that way). In this case, we’re telling the editor to open the poptracker GitHub repository folder, and to grab the relevant schema based on its name or folder location.  
This settings file is also a JSON file, which means that if you have more stuff after an entry (using another `[` after a `]`, or another `{` after a `}` ), you need to also have a comma to separate them. The editor will flag the error if it comes up, but because of how it parses that particular error it flags the NEXT line after the one that needs the comma. 

Whew! That’s a lot, but now all of the actual setup is done. From here, it’s just the actual creation of the pack. Exhausted yet? Go ahead and take a break if you need to, there’s no one pressuring you to do more except yourself.  
## **Pack Creation 101**
First off, you probably should have the pack documentation open for a quick skim through. You can find that at https://github.com/black-sliver/PopTracker/blob/master/doc/PACKS.md  
It’s comprehensive documentation on everything, and individual items are clear – but some things require context first, which is what we’re here for.
The most important things to note are all in the first few lines.  
 * Everything a pack provides is done through Lua, starting at scripts/init.lua
 * Data (that is loaded through Lua) is stored as JSON files inside the pack.  
 * Images (that are referenced in JSON files) are PNGs, GIFs, or JPEGs inside the pack.  
 * *Most* strings/identifiers are case sensitive.  

If you want your pack to load ANYTHING, you need to make sure that the init.lua file includes it. You don’t necessarily need more Lua, but if you want to do nice things with a tracker it’s definitely worth trying to pick up a little bit. Don’t be like me – the first thing I did was skim past the first few lines of documentation, deleted the scripts folder (assuming that I would just do non-scripty things), and only realized my mistake after trying to figure out what actually tells poptracker which bits to load and why.  

>There’s no Lua information because I know basically nothing about Lua, and no info is better than bad info. 

From here, there’s a few ways to build your tracker. You can either start with a functional slice or just YOLO it and build from the bottom up. The functional slice method will feel slow to start with, since you’ll be jumping around for a bit until you have a proof of concept up and running. The full build will have little feedback until the end, so if you made a mistake early then you might have a lot of mistakes to fix. Whichever methodology you choose is fine, but you’re going to have to weigh the pros and cons.  

> **JSON formatting in a nutshell**  
There’s really four things that you’re likely to see in a JSON file. Curly braces, {}, square brackets, [], text with quotes around it, “example”, and numbers. Curly braces are Objects (or dicts, which they mostly act like), square brackets are Arrays, and generally mean that the stuff between them is all treated as one thing, and text is part of a dict pair (EG. `“Key” : “Value”` or `“flags”: [flags go here]`  ). `true` and `false` (note lack of capitalization) are allowed to exist on their own, all other text requires a pair of quotes around it. If you mouse over text, the Schema should give you some contextual info about it. If you’re going to continue adding things at the same level (IE going from [] to another [] or {} to another {}, you’ll need to make sure there’s a comma after the closing symbol. VSCode should bring it to your attention when you forget, but (as mentioned earlier) it will flag on a line **after** the missed comma.  
### **Things that can be done immediately**
Go ahead and edit manifest.json. Give your pack a name, update the game name to your game, pick a UID that someone else hasn’t used, and slap your name on the author field. Congrats, it's a pack! If you have a plan to include just the item tracker vs. the full map and item tracker, you can leave the variants bit be. If your game includes major variations from the default settings (EG. Inverted Mode in LttP), then you’ll want to have a variant mode for that. The name after “variants” is what you’ll use internally in the pack to call the bits you need later, and the “display_name” is exactly what it says on the tin – the options that Poptracker will display when you click on your pack’s name. While you’re at it, update the Readme too. GitHub will automatically display that when people load up your repository, so have a blurb about what game it is, current limitations etc.

Now it’s time to get into the actual pack creation details.


### **What’s Where / Getting Started / Images**
As the documentation mentioned, the init.lua script grabs the .json files, and figures out everything with the information contained in them. 
 
If you don’t know where to start, items and maps don’t require anything else done first. Any items that have logical implications need to have an item created for them. Anything that you might have to check your inventory for would also be a good candidate. Anything that you never need later should generally not be an item. Bosses that you have to kill can also be an Item – this way they can have a nice icon instead of just being a random chest.

You can start with placeholder images for items. For Maps, best practices are to use a quick placeholder to make sure they’re loading, and you’ll want to have them finalized by the time you’re doing a second pass on the locations file. There's not a requirement to have placeholder images, but iterating on a quick test is generally a good way to prevent major unforseen issues.

Reminder again, **valid image types are PNG, GIF, and JPEG.** Generally speaking, you should prefer to create static images as PNG over JPEG since they can include transparency and are lossless. If you’ve already got something in another valid format then it’s probably fine as-is. Item image sizes should generally be 32 x 32, though up to 64 x 64 is still fine if you have text that needs to be readable.  
- For Future You’s sanity, please pick image names that are concise, descriptive and separate words with an underscore if you need multiple words to be descriptive (eg map_name). This isn't a hard rule, but it'll make life easier for anyone who needs to read the pack later on.
### **Items.json**
Once you have images, you’re ready to start adding the items to items.json  
The example pack is fantastic for creating these, and it’s going to give you 90% of what you need just by reading it. Here are a few notes that might not be immediately obvious from looking at them.  
 * The Name field will be displayed in Poptracker if the user hovers over the item.
 * Toggle is your standard “Have or don’t have” item. Most things will be these.
 * “Codes” are REALLY IMPORTANT. These are what you’re going to use for the item’s name in logical access rules and in the layouts section.
 * For Progressive Items, “Allow Disabled” means that there’s a None option automatically generated by graying out the first stage’s image. 
     * “inherit_codes” is on by default (progressive items keep previous level’s codes) when this entry is not present, the example disables it to show that it exists. This is useful in the event that progression means not being able to do something afterwards (glitches requiring a specific item, locking out an area until further progression).
 * “increment” exists as an additional flag for items where you always add fixed multiples at a time, and “type”:”consumable” for things that expect to be removed.  

There’s a lot more options, but it’s all better explained by the proper documentation for other use cases. This really just covers the bare minimum common stuff.
Sometimes, less is more. Here's an example of a bare-minimum item.
```
    {
        "name": "Macguffin",
        "type": "toggle",
        "img": "images/test.png",
        "codes": "KeyItem"
    }
```
It's got a name, an image, a code for use in the logic later, and the user can click it on and off. A lot of items will only need the basics, there's nothing wrong with that.

### **Maps.json**
If you’re doing this in order, you might think this is pretty similar to the items.json layout overall – that’s correct! These JSON files are just a flat structure with a bunch of things to tell the Lua script to load for other files to actually use. Some games will have a bunch of maps (we’ll go over organizing them nicely as tabs in layouts), and some games only need a few to cover everything. In either case, you just need a name for referencing in the code (display names are also handled as part of layouts, so don’t feel obligated to make it nice. As with the items, concise but descriptive is the way to go) an image, a location size, and a border thickness. Those last two things are actually the size of the green Check boxes that we’ll place on the map – you’re free to play around and see what feels reasonable for your maps. The default is good, but some highly-detailed maps might be better served by shrinking the boxes slightly – it’s up to you!

### **Locations.json**
Alright, here’s where things get significantly less deterministic on how you’re going to need to proceed. Here’s a few things that I wish I knew before diving right into the deep end.
 * Locations are in two sections – the first is the map information and the second is the actual checks at that location. If you have multiple checks at one location, then you’ll have multiple {} entries in the “Sections” portion that will be comma separated. “item_count” is for generic chests. “hosted_item” is for “items” you created earlier. 
 * **Don’t do this all manually.** If you can pull a list of locations from your randomizer’s documentation/source code, then automating most of the location creations is only a simple (seriously!) script away. Use your favorite language if you have one, or learn a couple basic tricks in Python if you don't. Anything to keep you from having to manually copy+paste+tweak 100+ locations.
 * You can nest the sections as deeply as it makes logical sense. If area C needs you to get to area B, which needs you to get to area A? C can be inside of B which is inside of A. Use the parents and children to help organize your data, rather than having to figure out each and every single check’s logic individually.
    * Child locations inherit the access_rules of their parent location. In the above example - any rules you add to A are also added to B and C, even if you don’t explicitly add the item requirements in B and C’s access_rules. You can still include the item requirements in the access_rules if you think it contributes to readability.

#### **How access_rules work, in brief:**  
  *	You can put any number of item codes in a single quote block, separated by commas, to make all items listed logically required to access that location.  
   `"access_rules":["Item1,Item2"]` would require Item1 AND Item2
  * You can comma separate multiple quote blocks to have alternate access requirements for an area.
  `"access_rules":["Item1","Item2"]` would require Item1 OR Item2 to be accessible.
    * You can mix and match both of those as needed.
    `"access_rules":["Item1,Item2","Item1,Item3"]` would require Item1 and one of Item2 or Item3 to be accessible.
    * If you have a complicated set of access requirements, it will likely be much cleaner to set up logical rules for combinations of items instead of having each item listed individually. 
* You can use [ ] around an item name in those quote blocks to flag the item requirement as optional. If other requirements are met, that will turn the check yellow (can be acquired but is out of logic).
* You can use {} in a quote block on item requirements to indicate that a location can be seen but not collected, indicated by a blue color on the tracker.  Ex: `“{}”` means you can always see the item.
 * In the example locations.json, Example Location 1 shows how to put a check on two different maps and keeping them linked.
 * If you want to set a custom image to replace the generic chest, you can do so on the lowest level item (which would then apply it to all checks). The tags to do so are `“chest_unopened_img”` and `“chest_opened_img”`, and they need a colon and a path to the image you want to use.
 * There’s a flag for clearing multiple chests with a single click. It’s called `“clear_as_group”`. Set it to True if you want to do so, set it to False to go one at a time. By default it’s True, but you can set it to False in a parent and it should be inherited by the children too.

When creating locations, I set the X and Y coordinates to a specific position in the top-left corner of the screen – that way it was immediately obvious which checks still needed to be correctly placed later. The X and Y coordinates are pixels, and the fastest simple way I found to get those values was to just open up the images in Paint and mouse over the intended spot. Paint displays your cursor’s X and Y coordinates along the bottom bar.

### **Layouts.json**
Layouts are the most complicated bit, because they’re completely modular. Want to have your tracker’s items hanging out on a specific side of the window? You can do that. Want to display 27 maps nicely? You can stick them in tab groups with tab groups inside. Want to have it display your items in such a fashion as to summon Cthulu? I can’t tell you how to do that, but you probably can if you do it wrong enough.  
Here’s a few notes to help you reach enlightenment understand a bit of what’s going on.   
* There’s three files in the layouts folder for a reason. 
* Broadcast.json exists to define what’s in the broadcast window (the little one that just has items). 
* This particular Items.json exists to define a nice little table for the items that live in items/items.json. it’s a bracketed set of bracketed sets – the inner brackets define what’s in a row, and the outer brackets define what’s in a column.
* tracker.json exists to define everything that poptracker shows you.
* In items.json, you can define multiple item grids. The first thing in quotes is the name you’re going to be using later in tracker.json
* Broadcast.json  should just reference any tables you created in items.json, and then you can basically call it a day there.
* Tracker.json has two sections, and you’ll want to make your changes similar in both of them. The tracker will pick tracker_horizontal or tracker_vertical based on the window size ratio – wider than 1:1 will pick horizontal and taller than 1:1 will pick vertical. The bit to change to put the dock on the correct side is next to the second “type”: “dock”, change “dock”: to “bottom” or “left” to put the items dock where it says.  
* Most of this can be left alone, and the parts that you do need to change should at least seem familiar. “key” is the name you gave to the item grid over in layouts/items.json
* “title” is what Poptracker will display at the top of the section / for the name of the map tab.
* “maps” is asking for the map names that you set way back in maps/maps.json. if you include multiple names here, poptracker will display the images side-by-side.
* If you need to have multiple levels of tabs, you can either have them all in the tracker file (gets messy fast), or you can create a new json file to define tab groupings. A partial example of the 2nd option:
```
"overworld_maps": {
      "type": "tabbed",
      "tabs": [
        {
          "title": "Map 1 Display Name",
          "content": {
            "type": "map",
            "maps": [
              "Map1_code"
            ]
          }
        },
        {
          "title": "Map 2 Display Name",
          "content": {
            "type": "map",
            "maps": [
              "map2_code"
            ]
          }
        }  
```
And what it looks like back in tracker.json to include that tab group of groups.
> This example was used to group 10 maps into 3 main tabs.  
```
{
			  "type": "tabbed",
			  "tabs": [
				{
				  "title": "Overworld Locations",
				  "content": {
					"type": "layout",
					"key": "overworld_maps"
				  }
				},
				{
				  "title": "Underworld Locations",
				  "content": {
					"type": "layout",
					"key": "underworld_maps"
				  }
				},
				{
				  "title": "Dungeon Locations",
				  "content": {
					"type": "layout",
					"key": "Dungeon_maps"
					  }
				}
			  ]
			}


```
**Important** – if you did opt to create another JSON file, go back and add it to scripts/init.lua
### **Finishing up**
And now’s the point where you get to put your pieces into action, and see what’s broken. Copy your whole repository folder into poptracker’s Packs folder, open up Poptracker, and see how it looks. You can make changes to the files in the packs folder with Poptracker open and press the Refresh button to load your changes. From here, double-check that logic functions the way you expect it to when items are added / removed, confirm that your maps are readable, and make sure the layouts don’t waste a bunch of space – not everyone has a nice monitor to see tiny pixels.  
If you created a variant (way back in the Manifest.json), then you’re going to need to create the files that get loaded in place of the default JSON files – but that’s really just more of what you’ve already been doing. Read the documentation in the example pack, and you’ll be good to go.

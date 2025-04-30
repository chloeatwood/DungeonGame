# 🧱 DungeonGame

A terminal-based dungeon crawler game built in **C and C++**, where the player explores randomly generated dungeons, engages with monsters, and collects items—all from the command line!

## 🚀 Features

- 🗺️ **Procedurally Generated Dungeons**: Every playthrough is unique.
- 👤 **Player and Monster Turn Logic**: Managed with a custom heap-based event system.
- 🔦 **Fog of War**: PC remembers explored areas, even when they're out of sight.
- 🎒 **Item Mechanics**: Pick up and equip weapons, armor, and other loot.
- 👾 **Monster AI**: Intelligent behavior including pathfinding and engagement.
- 🧪 **Debug Tools**: Teleport (`g`), reveal map (`f`), and more for development/testing.

## 💻 Technologies

- **C & C++**: Core logic written in C, ported to C++ for modularity and OOP design.
- **ncurses**: Used for terminal rendering and keyboard input handling.
- **Custom Min-Heap**: Event queue for managing turn order.
- **OOP Design**: Inheritance-based character system for PC/NPCs.

  ## Controls

### Movement

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `7` or `y`      | Move PC one cell up-left.          |
| `8` or `k`      | Move PC one cell up.               |
| `9` or `u`      | Move PC one cell up-right.         |
| `6` or `l`      | Move PC one cell right.            |
| `3` or `n`      | Move PC one cell down-right.       |
| `2` or `j`      | Move PC one cell down.             |
| `1` or `b`      | Move PC one cell down-left.        |
| `4` or `h`      | Move PC one cell left.             |

### Stairs

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `>`             | Go down stairs (if standing on down staircase). |
| `<`             | Go up stairs (if standing on up staircase). |

### Resting

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `5`, `space`, or `.` | Rest for a turn (NPCs still move). |

### Monster List

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `m`             | Display a list of monsters in the dungeon with their symbol and position relative to the PC. |
| `up arrow`      | Scroll monster list up (if not at the top). |
| `down arrow`    | Scroll monster list down (if not at the bottom). |
| `escape`        | Return to character control when viewing the monster list. |

### Quit

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `Q`             | Quit the game.                     |

### Equipment and Combat

| Key(s)          | Action                             |
|-----------------|------------------------------------|
| `w`             | Wear an item. Prompts the user for a carry slot. If an item of that type is already equipped, items are swapped. |
| `t`             | Take off an item. Prompts for equipment slot. The item is moved to an open carry slot. |
| `d`             | Drop an item. Prompts user for a carry slot. The item is placed on the floor. |
| `x`             | Expunge an item from the game. Prompts the user for a carry slot. The item is permanently removed from the game. |
| `i`             | List the player's inventory.      |
| `e`             | List the player's equipment.      |
| `I`             | Inspect an item. Prompts user for a carry slot, then displays the item’s description. |
| `L`             | Look at a monster. Enter targeting mode. Select a visible monster with `t` or abort with `esc`. When a monster is selected, its description (and other information) is displayed. |


> Requires a C++ compiler (e.g., `g++`) and `make`. On Windows, use **MinGW** or **WSL**.

### Linux/macOS:
```bash
make
./rlg327
```

### Windows (MinGW):

```bash
mingw32-make
dungeon.exe
```

### Windows (WSL):

```bash
sudo apt install build-essential libncurses-dev
make
./dungeon
```



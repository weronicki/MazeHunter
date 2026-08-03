# 👻 Maze Hunter

A first-person horror maze game for the **ESP32-C6 SuperMini** and a **128×64 SSD1306 OLED**.

Inspired by classic games like **Wolfenstein 3D**, Maze Hunter uses a real-time raycasting engine to render a procedurally generated maze from a first-person perspective on a tiny monochrome display.

Every game generates a completely new maze. Your objective is simple:

> **Find the exit before the Hunter finds you.**

---

## Features

- 🎮 Real-time 3D raycasting engine
- 🧩 Procedurally generated maze (different every game)
- 👻 Intelligent enemy AI
  - Line-of-sight detection
  - Pathfinding
  - Random wandering
  - Continuous pursuit
- 🚪 Textured exit door
- 👁️ Sprite rendering with proper wall occlusion (Z-buffer)
- 🗺️ Toggleable minimap
- 💀 Game Over / Victory screens
- ⚡ Runs entirely on an ESP32-C6

---

# Demo

*(Add screenshots or GIF here)*

Example:

```
+-----------------------------+
|                             |
|        (game screenshot)    |
|                             |
+-----------------------------+
```

---

# Hardware Requirements

- ESP32-C6 SuperMini
- SSD1306 OLED Display (128×64, I2C)
- 8 × Momentary Push Buttons
- Breadboard or custom PCB

---

# Wiring

## OLED Display

| OLED | ESP32-C6 |
|------|-----------|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO4 |
| SCL | GPIO7 |

---

## Buttons

All buttons connect between the GPIO pin and **GND**.

The firmware uses the ESP32 internal pull-up resistors (`INPUT_PULLUP`), therefore **no external resistors are required.**

| Button | GPIO |
|--------|------|
| UP | GPIO0 |
| DOWN | GPIO1 |
| LEFT | GPIO2 |
| RIGHT | GPIO3 |
| A | GPIO20 |
| B | GPIO19 |
| MENU1 | GPIO18 |
| MENU2 | GPIO14 |

---

# Controls

| Button | Action |
|---------|--------|
| A | Move Forward |
| B | Move Backward |
| LEFT | Turn Left |
| RIGHT | Turn Right |
| MENU1 | Generate New Maze |
| MENU2 | Toggle Minimap |

---

# Gameplay

Each new game creates a completely random maze.

The player starts near one corner of the maze while the exit is located elsewhere.

Somewhere inside the maze lurks the Hunter.

The Hunter is capable of:

- detecting the player when visible
- navigating around walls
- chasing using pathfinding
- wandering when it loses sight of the player

Reach the exit before the Hunter catches you.

---

# Technical Details

## Graphics

- 128×64 SSD1306 OLED
- Software raycasting renderer
- DDA ray traversal
- Perspective correct wall projection
- Distance corrected rendering
- Sprite projection
- Z-buffer for sprite occlusion

---

## Maze Generation

The maze is generated at startup using an iterative **Depth-First Search (DFS)** algorithm.

Advantages:

- every maze is unique
- no isolated areas
- guaranteed path from start to exit

---

## Enemy AI

The Hunter combines several behaviors:

### Line of Sight

The Hunter checks whether the player is directly visible using ray traversal.

---

### Pathfinding

When chasing the player, a Breadth-First Search (BFS) distance map is generated allowing the Hunter to always choose the shortest available path.

---

### Wandering

When the player is not visible, the Hunter randomly explores the maze.

---

### Collision

Touching the Hunter immediately ends the game.

---

# Project Structure

```
MazeHunter/
│
├── MazeHunter.ino
├── README.md
└── images/
    ├── gameplay.png
    ├── minimap.png
    └── wiring.png
```

---

# Libraries

Install using the Arduino Library Manager.

- Adafruit GFX Library
- Adafruit SSD1306

---

# Memory Usage

The game comfortably fits inside the ESP32-C6.

Major memory consumers include:

- OLED framebuffer
- Z-buffer
- BFS distance map
- Maze data
- Sprite data

No dynamic memory allocation is used during gameplay.

---

# Future Ideas

- Multiple enemy types
- Flashlight mode
- Sound effects
- Footstep audio
- Difficulty levels
- Multiple floors
- Collectable keys
- Locked doors
- Better wall textures
- Animated sprites
- Saving high scores
- Battery-powered handheld version

---

# Building

Clone the repository:

```bash
git clone https://github.com/yourusername/MazeHunter.git
```

Open

```
MazeHunter.ino
```

Compile and upload using the Arduino IDE.

---

# License

MIT License

Feel free to use, modify and improve this project.

---

# Acknowledgements

Inspired by:

- Wolfenstein 3D
- DOOM
- Lode Vandevenne's Raycasting Tutorial
- The ESP32 open-source community

---

If you build one, I'd love to see it!

⭐ Star the repository if you enjoyed the project.

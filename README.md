# RetroKart 3D
**A Software-Rasterized First-Person Pseudo-3D Racing Game in Qt & C++**

---

## 🏎️ Overview
**RetroKart 3D** is an arcade first-person racing game developed using **pure pixel manipulation** in C++ and Qt (no OpenGL, no vector graphics, no hardware shaders). The graphics engine is powered by an implementation of **Mode 7 Scanline Inverse Perspective Mapping**, **Perspective-Correct Billboard Sprites**, and **Parallax Horizon Scrolling**.

For the exhaustive mathematical formulation and viva preparation, refer to **[PROJECT_FORMULATION.md](PROJECT_FORMULATION.md)**.

---

## 🎮 Controls
| Key | Action |
| :--- | :--- |
| **W / Up Arrow** | Accelerate |
| **S / Down Arrow** | Brake / Reverse |
| **A / Left Arrow** | Steer Left |
| **D / Right Arrow** | Steer Right |
| **R** | Reset Kart to Starting Line |

---

## 🏁 Track Levels
1. **Level 1: Donut Plains (Easy)** — Wide tarmac raceway, gentle curves, forgiving grass borders.
2. **Level 2: Choco Valley (Medium)** — Sharper hairpins, dirt terrain with drift physics, and mud puddles.
3. **Level 3: Bowser's Castle (Hard)** — Tight 90° corners, fortress walls, and lethal molten lava borders.

---

## 🛠️ Build & Run Instructions
1. Open `RetroKart3D.pro` in **Qt Creator**.
2. Select the desktop kit (e.g., **Desktop Qt 6.11.1 MinGW 64-bit**).
3. Click **Run** (`Ctrl + R`).

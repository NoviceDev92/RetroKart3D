Deliver significant improvements to RetroKart3D for a project showcase today, including a user menu, character selection, AI opponents with fixed item targeting, multiplayer support, and improved physics and tracks.

Working directory: d:\Qt\RetroKart3D
Integrity mode: demo
The team must NOT: copy code from existing open-source projects for core logic, run external scripts or delegate execution to other tools

## Requirements

### R1. User Menu and Character Selection
Add a title and main menu flow at startup with options for Play vs AI, Multiplayer, and Practice, followed by a character selection screen featuring distinct characters with different karts and stats.

### R2. AI Opponents and Item Targeting
Add active AI opponents so races are fully populated. Fix item targeting (including bananas, green shells, and ink blooper) so items affect opponents rather than the player.

### R3. Multiplayer Support
Implement multiplayer capability so players can race together, choosing between network play or local multiplayer based on what best fits today's deliverable without over-complication.

### R4. Physics and Track Enhancements
Improve driving physics feel with responsive WASD arcade handling and drift/braking feel. Enhance tracks with interactive hazards (such as spewing lava geysers and chocolate fountains/candies) and a functional minimap in the top-left corner.

### R5. Build Stability and Performance
The project must compile cleanly with Desktop Qt 6.11.1 MinGW 64-bit on Windows and run smoothly at 60 FPS without crashing.

## Acceptance Criteria

### Compilation & Stability
- [ ] Compiles cleanly using Desktop Qt 6.11.1 MinGW 64-bit without errors or missing dependencies.
- [ ] Game runs stably at 60 FPS without crashing during menu navigation, character selection, and active races.

### Menu & Character Flow
- [ ] Launching the application presents a main menu allowing selection of Play vs AI, Multiplayer, or Practice.
- [ ] Character selection presents distinct characters with observable differences in kart appearance and performance stats.

### Gameplay & AI
- [ ] Starting a race in Play vs AI populates the track with active AI opponent karts that navigate the course.
- [ ] Deployed items (bananas, green shells, ink blooper) target and affect opponent karts upon contact rather than self-inflicting effects on the player.

### Multiplayer
- [ ] Multiplayer mode allows two or more players to participate in a race together (via LAN/sockets or local controls).

### Tracks & Visuals
- [ ] Enhanced track hazards (lava geysers on Bowser Castle, chocolate fountains/candies on Choco Valley) trigger visual animations and interact with karts.
- [ ] A minimap is displayed at the top-left of the HUD showing track outline and real-time kart positions.

# Nestris x86

> This is a fork of [rosskidson/nestris_x86](https://github.com/rosskidson/nestris_x86). It adds lower input lag, run ahead, trainer modes, a score history and a few fixes. See [What's new in this fork](#whats-new-in-this-fork). Everything else is the original game.

### Summary
This is a reimplementation of the original 1989 NES Tetris game using modern c++ compilers for modern computer hardware.  Nothing from the NES is being emulated here, this is a native implementation of the game for Windows / Linux / MacOs.  As a result there is no emulator lag.  However being a port, it is not identical, there are some aspects of the original game missing, whilst other features have been added.  It is the intent of this project to be as close as possible to the original NES game in terms of gameplay mechanics.

### What's new in this fork
- **Lower input lag**: frames are shown about 1 ms after the input is read, instead of about 17 ms. See [Input latency](#input-latency).
- **Run ahead**: shows the game up to 6 frames ahead to compensate for display lag. See [Run Ahead Frames](#run-ahead-frames).
- **Trainer modes**: maxout, quicktap, garbage and drought. See [Game modes](#game-modes).
- **Score history**: every game is saved, with a progression plot and best scores per mode and start level. See [Score history](#score-history).
- **Lag test**: a screen for measuring your setup's input lag with a slow motion camera. See [Lag Test](#lag-test).
- **Soft reset**: A + B + Start during a game returns to the title screen.
- **Fullscreen**: start with `--fullscreen` (or `-f`).
- **Background option**: hide the background pattern in game.
- **Scoring fix**: line clears score (level + 1) × 40 / 100 / 300 / 1200 points, as on the NES. Before, the multiplier was the level, so scores were too low.
- **TRT fix**: the tetris rate is the percentage of lines cleared with tetrises, as in CTWC broadcasts. Before, it was the percentage of the score.

The changes have been tested on Linux only.

### Features
- Very low input latency
- Identical DAS mechanics
- Controller support (Windows and Linux)
- NTSC and PAL modes
- Identical rotation system
- Same RNG piece distribution

#### Additional features
- Customizable DAS speeds
- Customizable framerate
- Customizable RNG (e.g. uniform, 7-bag)
- CTWC "Trey Vision" statistics (tetris rate, droughts, burn etc.)
- Hard drop (may be enabled/disabled)
- Wall kick (may be enabled/disabled)
- DAS charge visualization
- Run ahead, trainer modes, score history and lag test (this fork)

#### Missing features
- Type B
- Rocket launch endings
- Original music
- Save keyboard/gamepad key bindings

The features that are missing are not for any technical reason, the focus was simply placed on other features.  Feature requests or suggesstions are welcome. Simply make a issue on the project github page, or just email me directly: ross dot kidson at gmail dot com

### Input latency
As mentioned already, there is no emulation taking place, and therefore no emulation lag. Input lag experienced will be due to usb polling and monitor response times, with the latter being the likely culprit. Comparing tetris on a physical NES + CRT TV to this software using a gaming monitor the input lag is noticeable, however, at least in the developer's opinion, certainly very playable.  Mileage will vary depending on operating system, monitor, bluetooth vs usb controller etc. etc.

In this fork the game waits for the next frame *before* reading the input, instead of between drawing a frame and showing it. A frame now reaches the display about 1 ms after the input was read, instead of about 17 ms (measured at 60 Hz). With a 240 fps camera, the total lag from button press to picture on a 60 Hz laptop panel measured about 17 ms. That is close to the minimum for a 60 Hz game on a 60 Hz display: on average half a frame waiting for the input to be read, and half a frame for the display to draw that part of the screen.

To reduce lag further:
- Run in fullscreen (`--fullscreen`). On desktops with a compositor, also turn off compositing for fullscreen windows. On Linux Mint Cinnamon this is *System Settings > General > Disable compositing for full-screen windows*.
- Use the game or low latency mode of your monitor or TV.
- Use [Run ahead](#run-ahead-frames) to compensate for the lag of your display.

### Download / Installation

This game depends on SDL-mixer

Linux:
```
sudo apt install libsdl-mixer1.2-dev libsdl1.2-dev libsdl-image1.2-dev
```
MacOS:
```
brew install libpng sdl_mixer
```
Windows:
SDL has been packaged in the release for Windows.

Binaries of the original game are available under its releases. They do not include the changes of this fork, so build from source for those:
https://github.com/rosskidson/nestris_x86/releases/tag/v0.7

### Building

#### Checkout
The repository has subrepositories, therefore clone recursive:
```
git clone --recursive https://github.com/blast664/nestris_x86.git
```
If you are reading this after having checked out without the extra flag, simply run these commands:
```
git submodule init
git submodule update
```

#### Linux
```
sudo apt install cmake libsdl-mixer1.2-dev libsdl1.2-dev libsdl-image1.2-dev libyaml-cpp-dev libpng-dev libgl-dev libx11-dev
```
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
```
#### MacOS
```
brew install libpng sdl_mixer
```
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
```
#### Windows
SDL for windows has been included in this repository. After checking out the code open the directory in Visual Studio and configure using CMake.

### Running
```
./nestris_x86              # in a window
./nestris_x86 --fullscreen # fullscreen, or -f
```
Press Q to quit.

The game stores its settings in `config.yaml` and the score history in `game_history.csv`, both in the directory the game is started from. Start it from the same directory to keep them.

### Controls
The default keyboard controls are the arrow keys, X (A, rotate clockwise), Z (B, rotate anticlockwise) and Enter (Start).

- Start: pause
- A + B + Start: return to the title screen (soft reset). The game is saved in the score history.
- On the level screen, A + Start starts ten levels higher, e.g. A + Start on level 8 starts level 18.

### Game modes
On the level screen, press right past level 4 to reach the menu with OPTIONS, MODES and HISTORY. MODES selects the game mode and its setting. A trainer mode's name replaces A-TYPE on the level screen.

- Normal - The regular A-type game.
- Maxout - The game starts with a set score (0 to 1,500,000). Use it to practise the end of a maxout game.
- Quicktap - Only long bars spawn, and there is a permanent tower of a set height (0 to 18) in the second column. Each bar is removed after it locks, so every bar starts on the same board. Use it to practise tapping a long bar over the tower to the first column.
- Garbage - A permanent floor of a set height (1 to 15) at the bottom of the playfield. It is never cleared, so the playfield is shorter.
- Drought - Long bars appear at a set percentage (0 to 100 %) of their normal rate. A long bar that is removed is replaced by a random other piece.

### Score history
Every game is appended to `game_history.csv`, one line per game. The file can be opened with a spreadsheet. It stores the time, mode and its setting, start and end level, lines, score, start score, tetris rate, pieces, duration, run ahead frames, and whether the game ended by topping out or a soft reset.

Topped out games and soft reset games count for the scores, except soft reset games without a cleared line, as restarting after a bad start is common. Scores are kept separately per game mode and start level:

- The level screen shows the best three games for the selected level. Holding A shows the best games ten levels higher, the level A + Start would start.
- TOP in game shows the best score for the mode and start level.
- HISTORY shows the number of games, the best score and the average score of all games, the last 100 and the last 10 games. A plot shows the score of each of the last 184 games and the average of every 10 games. Left/right selects the mode, up/down the start level.

### Description of Game Options

##### Configure Keyboard
Change the keyboard bindings.  The new bindings will not be in use until after selecting 'accept'

##### Configure Gamepad
Change the bindings of a usb/bluetooth controller. The new bindings will not be in use until after selecting 'accept'

##### Lag Test
Three bars at the top, middle and bottom of the screen light up on the frame the game registers the A button, and stay lit while it is held. They use the same input and drawing path as the game. Film the button and the screen together in slow motion. The lag is the number of video frames between the button going down and the bars lighting up (at 240 fps, each video frame is 4.2 ms). Average about 10 presses.

##### DAS Profile
Despite the name, this doesn't just apply to the DAS, but also the level speeds.  Using PAL will result in a faster lvl 18 and the lvl 19 kill (thrill) screen.

- NTSC - North America/Japan 60hz NTSC version
- PAL - Europe 50hz version
- None - No das.  This is useful for practicing tapping
- NTSC DAS, without any initial delay
- Custom - customize the game frequency, das initial delay and das repeat delay

##### Hard drop
Enables/disables hard drop.  Use the up arrow to do a hard drop.

##### Wall Kick
Enables/disables wall kicks.

##### Statistics
- NES - The tetromino counts as in the original tetris game
- Trey V - 'Trey Vision' - these are the statistics that are to be seen in a typical CTWC match, plus a few others:
  - BURN - How many lines have been cleared from singles, doubles or triples since the last tetris
  - TRT - Tetris rate - The percentage of cleared lines that were cleared with tetrises
  - Long bar drought - How many pieces since that last long bar
  - DAS - DAS chain - This is a counter of how many pieces that are dropped without reseting the das counter to zero
  - ENTRY DL - Entry delay - This goes green when the entry delay, aka 'ARE' is active. During this time one may redirect DAS without resetting it
  - WALL CHR - Wall charge - This goes green when a wall charge is performed

##### Show DAS meter
Will show the das counter as a meter. Green means das is charged

##### Show Controls
Displays a small visual showing which keys are being pressed. Potentially useful for checking keyboard/gamepad layout.

##### Background
Shows or hides the background pattern around the panels in game. When hidden, the tetris flash is solid white outside the panels.

##### RNG Type
Here one can select different types of random number generators for the tetromino spawning.  In all cases the code attempts to use non deterministic random number generation hardware, if available.  This is typically available on modern computers.  If this is not available, the identical piece set will occur every time.

- NES - This is an approximation of the original implementation.  A reroll is performed once if a piece is repeated, and there is a very slight bias for and against certain pieces.
- UNIFORM - A true uniform distrubution across all tetrominos. No rerolls, no guarentees on drought lengths
- 7-BAG - Seven bag - This is the approach for modern tetris, where a 'bag' of all seven tetrominos is shuffled and then dealt.  When the first set is finished the next one will be shuffled.  Using this means there is guarenteed to be no bias across tetrominos, and it also means the maximum drought length is 14 (start of the first set, and end of the second set)

##### Run Ahead Frames
Shows the game 0 to 6 frames ahead, to compensate for the lag of your display. It takes effect when a new game is started. 0 is the original behaviour.

Each frame, the game is run with your input, saved, then run the set number of frames further assuming the pressed buttons stay pressed, and that frame is shown. Then the game goes back to the saved state for the next input. Things that happen over time, like gravity, DAS shifts, piece spawns and line clears, are shown that many frames early. A new button press still takes your display's lag to appear.

Things to be aware of:
- Releasing a held direction during DAS makes the piece snap back, by up to the set number of columns. The predicted frames had kept shifting it.
- Sounds of things that happen over time, like locking and line clears, play that many frames after their picture. Move and rotate sounds stay in sync.

1 frame is a good start. Higher settings compensate more lag, but make the snap back more visible.

### Acknowledgements / References
This was built using OneLoneCoder PixelGameEngine
- https://github.com/OneLoneCoder/olcPixelGameEngine
- https://www.youtube.com/channel/UC-yuWVUplUJZvieEligKBkA

There is a super detailed techincal analysis of the original assembly source code for nes tetris. This was extremely helpful to get all the fine details right.
- https://meatfighter.com/nintendotetrisai/

The tetris wiki was also very useful in providing details about das mechanics and the line clear animation.
- https://tetris.wiki/Tetris_(NES,_Nintendo)

The emulator of choice for development on this project is fceux:
- http://fceux.com/web/home.html

### Bugs, suggestions, feedback
For the original game: Any feedback is very welcome! Open an issue or simply email me directly: ross dot kidson at gmail dot com

The changes of this fork are maintained at https://github.com/blast664/nestris_x86.

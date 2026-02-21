# Case Opener

A CS-style case opening game built with **C++17** and **SFML 2.6**.

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./bin/CaseOpener          # Linux / macOS
bin\CaseOpener.exe        # Windows
```

**Dependencies (auto-fetched via CMake FetchContent):**
- SFML 2.6.1
- nlohmann/json 3.11.3 (single header)

---

## Controls

| Input        | Action                          |
|--------------|---------------------------------|
| Left click   | All UI interactions             |
| Scroll wheel | Inventory grid / trade-up picker|
| **M**        | Toggle audio mute               |
| **Escape**   | Close info panel                |

---

## Features

### Economy
- **Balance** displayed in header; starts at $1,000.
- **Case opening** deducts price × count from balance.
- **Sell items** in Inventory tab → credited to balance.
- **Free Case Reward** — circular countdown button bottom-right.
  Recharges every 30 minutes (edit `FreeCaseReward` constructor to change).

### Trade Up (tab 3)
- Select **3 items of the same rarity** from your inventory.
- Press **TRADE UP** → receive 1 item of the next rarity tier.
- Result wear = average input wear ± small random variance.
- Contraband items cannot be traded further.

### Animation
- Horizontal scrolling reel with **ease-out cubic** deceleration.
- Open 1 / 3 / 5 / 10 cases simultaneously (staggered starts).
- **Skip** button skips animation and shows results immediately.
- **Sparkle particles** fire on Classified+ reveals and trade-ups.
- All **buttons** have smooth lerp hover colour + outline brightening.
- CTA buttons (Open Case) have an idle **pulse glow**.

### Save System
- Saves to `save.json` next to the binary (atomic temp-rename write).
- Persists: balance, full inventory (id + wear), free-case cooldown timestamp.
- Unknown item IDs from old saves are skipped gracefully.
- Auto-triggered on: Collect All, Back to Menu, Save button.

---

## Architecture

```
src/
├── core/
│   ├── PlayerData      Legacy text save (superseded by JsonSave)
│   ├── JsonSave        JSON persistence via nlohmann/json
│   ├── AudioManager    8-slot sound pool, rarity-mapped reveals
│   └── FreeCaseReward  Real-time cooldown, Unix timestamp persist
│
├── items/
│   ├── Rarity          7-tier enum + constexpr weights/colours
│   ├── Wear            Float [0,1] → 5 tier bands + value multipliers
│   ├── Item            Instance: wear, value formula, lazy texture
│   └── ItemRegistry    Singleton def store (14 example items)
│
├── cases/
│   ├── Case            Pool, weighted RNG, wear bias per rarity, open()
│   └── CaseRegistry    5 cases $0.99–$24.99
│
├── gameplay/
│   ├── ReelAnimation   Single reel: ease-out cubic, viewport scissor
│   ├── MultiReelManager 1/3/5/10 reels, stagger, skip-all
│   ├── Inventory       Stable storage, sorted view, sell-by-view-index
│   └── TradeUp         3-same-rarity → 1 next-rarity, wear averaging
│
├── ui/
│   ├── Button          Smooth HoverEffect lerp, pulse, outline glow
│   ├── HoverEffect     Header-only: lerp animator + SparkleEmitter
│   ├── InventoryUI     Scrollable grid, inspect panel, sell
│   ├── CaseInfoPanel   Modal: animated odds bar chart, pool list
│   ├── TradeUpUI       3-slot picker, flash animation, result reveal
│   └── FreeCaseButton  Arc progress ring, gold pulse when ready
│
└── screens/
    ├── MenuScreen      Title + Play/Quit
    └── GameScreen      3 tabs: Open / Inventory / Trade Up
```

---

## Adding content

**New item** → `ItemRegistry::load()`: `registerDef({id, name, Rarity::X, baseValue, "assets/images/id.png"})`

**New case** → `CaseRegistry::load()`: `Case c("id", "Name", "assets/images/case_id.png", price)`

**Change free-case cooldown** → `GameScreen` constructor: `m_freeCaseReward(seconds)`

**New sound** → add `SoundID` enum value + filename in `AudioManager.cpp`, call `play(SoundID::X)`

---

## Save file format (`save.json`)

```json
{
  "version": 2,
  "balance": 1234.56,
  "free_case_cooldown_end": 1700000000.0,
  "inventory": [
    { "id": "ak47_redline", "wear": 0.043 },
    { "id": "karambit_fade", "wear": 0.002 }
  ]
}
```

# Font Assets

Place a single font file here named **`font.ttf`**.

## Recommendations (free, open-source)

| Font             | Style              | URL                                              |
|------------------|--------------------|--------------------------------------------------|
| **Rajdhani**     | Angular, techy     | https://fonts.google.com/specimen/Rajdhani       |
| **Exo 2**        | Clean sci-fi       | https://fonts.google.com/specimen/Exo+2          |
| **Share Tech Mono** | Monospace, gritty | https://fonts.google.com/specimen/Share+Tech+Mono |
| **Orbitron**     | Futuristic caps    | https://fonts.google.com/specimen/Orbitron       |

Download the Regular weight `.ttf` and rename it `font.ttf`.

## Fallback behaviour
If `assets/fonts/font.ttf` is not found, the engine tries:
1. `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf` (Linux)
2. `/System/Library/Fonts/Helvetica.ttc` (macOS)
3. `C:/Windows/Fonts/arial.ttf` (Windows)

The game still runs without a font, but all text will be invisible.

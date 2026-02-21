# Sound Assets

Place the following audio files in this directory.
Supported formats: **WAV**, **OGG/Vorbis**, **FLAC** (SFML limitation — no MP3).

| Filename                  | When played                                        | Suggested duration |
|---------------------------|----------------------------------------------------|--------------------|
| `reel_start.wav`          | Case lid opens / reel begins spinning              | 0.5–1 s            |
| `reel_tick.wav`           | Each card tick while spinning (short, loopable)    | 0.05–0.1 s         |
| `reel_stop.wav`           | Reel decelerates to stop                           | 0.5–1 s            |
| `reveal_common.wav`       | Consumer or Industrial item revealed               | 0.5–1 s            |
| `reveal_uncommon.wav`     | Mil-Spec or Restricted item revealed               | 0.8–1.5 s          |
| `reveal_rare.wav`         | Classified or Covert item revealed (dramatic)      | 1–2 s              |
| `reveal_contraband.wav`   | Contraband item — ultra rare fanfare               | 2–4 s              |
| `item_sell.wav`           | Item sold for cash                                 | 0.3–0.5 s          |
| `button_click.wav`        | Generic UI button click                            | 0.05–0.1 s         |

## Free sources
- https://freesound.org (CC0 / CC-BY)
- https://opengameart.org

## Notes
- Missing files are silently ignored — the game runs without sound.
- The AudioManager uses an 8-slot round-robin pool to allow overlapping sounds.
- Master volume defaults to 80 %. Press **M** in-game to toggle mute.

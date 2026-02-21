#include "AudioManager.hpp"
#include <iostream>
#include <array>

// ── Mapping: SoundID → filename ───────────────────────────────────────────────
// Place .wav or .ogg files in assets/sounds/ matching these names.
// Supported formats: WAV, OGG/Vorbis, FLAC  (SFML limitation: no MP3).
static const std::array<const char*, static_cast<int>(SoundID::COUNT_)> s_filenames
{{
    "assets/sounds/reel_start.wav",        // ReelStart
    "assets/sounds/reel_tick.wav",         // ReelTick
    "assets/sounds/reel_stop.wav",         // ReelStop
    "assets/sounds/reveal_common.wav",     // RevealCommon
    "assets/sounds/reveal_uncommon.wav",   // RevealUncommon
    "assets/sounds/reveal_rare.wav",       // RevealRare
    "assets/sounds/reveal_contraband.wav", // RevealContraband
    "assets/sounds/item_sell.wav",         // ItemSell
    "assets/sounds/button_click.wav",      // ButtonClick
}};

// ── instance ──────────────────────────────────────────────────────────────────
AudioManager& AudioManager::instance()
{
    static AudioManager s;
    return s;
}

// ── load ──────────────────────────────────────────────────────────────────────
void AudioManager::load()
{
    if (m_loaded) return;
    m_loaded = true;

    int loaded  = 0;
    int missing = 0;

    for (int i = 0; i < static_cast<int>(SoundID::COUNT_); ++i)
    {
        sf::SoundBuffer buf;
        if (buf.loadFromFile(s_filenames[i]))
        {
            m_buffers[i] = std::move(buf);
            ++loaded;
        }
        else
        {
            // Missing sound — warn once, continue without crashing.
            std::cerr << "[AudioManager] Missing: " << s_filenames[i] << "\n";
            ++missing;
        }
    }

    std::cout << "[AudioManager] Loaded " << loaded << " sounds"
              << (missing ? " (" + std::to_string(missing) + " missing)" : "")
              << ".\n";
}

// ── play ──────────────────────────────────────────────────────────────────────
void AudioManager::play(SoundID id, float volumeScale)
{
    if (m_muted) return;

    auto it = m_buffers.find(static_cast<int>(id));
    if (it == m_buffers.end()) return; // sound file was not loaded — silent

    sf::Sound& snd = nextFreeSound();
    snd.setBuffer(it->second);
    snd.setVolume(std::clamp(m_masterVolume * volumeScale, 0.f, 100.f));
    snd.play();
}

// ── playRevealForRarity ───────────────────────────────────────────────────────
void AudioManager::playRevealForRarity(int rarityInt)
{
    SoundID id;
    switch (rarityInt)
    {
        case 0: case 1: id = SoundID::RevealCommon;     break;
        case 2: case 3: id = SoundID::RevealUncommon;   break;
        case 4: case 5: id = SoundID::RevealRare;       break;
        case 6:         id = SoundID::RevealContraband; break;
        default:        id = SoundID::RevealCommon;     break;
    }
    play(id);
}

// ── setMasterVolume ───────────────────────────────────────────────────────────
void AudioManager::setMasterVolume(float vol)
{
    m_masterVolume = std::clamp(vol, 0.f, 100.f);
}

// ── setMuted / toggleMute ─────────────────────────────────────────────────────
void AudioManager::setMuted(bool muted) { m_muted = muted; }
void AudioManager::toggleMute()         { m_muted = !m_muted; }

// ── nextFreeSound ─────────────────────────────────────────────────────────────
// Round-robin through the pool; prefer a stopped slot, else evict oldest.
sf::Sound& AudioManager::nextFreeSound()
{
    // First: look for a stopped slot starting from current position
    for (int i = 0; i < POOL_SIZE; ++i)
    {
        const int idx = (m_poolIdx + i) % POOL_SIZE;
        if (m_pool[idx].getStatus() == sf::Sound::Stopped)
        {
            m_poolIdx = (idx + 1) % POOL_SIZE;
            return m_pool[idx];
        }
    }
    // All playing — evict oldest (round-robin)
    sf::Sound& snd = m_pool[m_poolIdx];
    snd.stop();
    m_poolIdx = (m_poolIdx + 1) % POOL_SIZE;
    return snd;
}

#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <unordered_map>
#include <string>
#include <memory>

// ── SoundID ───────────────────────────────────────────────────────────────────
// Every in-game sound event has a named ID.
// The AudioManager maps these to .wav / .ogg files in assets/sounds/.
// ─────────────────────────────────────────────────────────────────────────────
enum class SoundID
{
    ReelStart,       // case lid open / reel begin spinning
    ReelTick,        // card tick while spinning  (play repeatedly, short)
    ReelStop,        // reel decelerates to stop
    RevealCommon,    // Consumer / Industrial reveal
    RevealUncommon,  // MilSpec / Restricted reveal
    RevealRare,      // Classified / Covert reveal — dramatic sting
    RevealContraband,// Contraband — ultra rare fanfare
    ItemSell,        // coin/cash sound on sell
    ButtonClick,     // generic UI click
    COUNT_           // sentinel — keep last
};

// ── AudioManager ──────────────────────────────────────────────────────────────
// Loads sound buffers once and plays them on demand.
// All methods are safe to call even if no sound file exists —
// missing sounds are silently skipped so the game runs without an audio pack.
//
// Usage:
//   AudioManager::instance().load();
//   AudioManager::instance().play(SoundID::ReelStart);
//   AudioManager::instance().setMasterVolume(80.f);  // 0–100
// ─────────────────────────────────────────────────────────────────────────────
class AudioManager
{
public:
    static AudioManager& instance();

    // Load all sound files from assets/sounds/.
    // Safe to call multiple times; subsequent calls are no-ops.
    void load();

    // Play a sound once (fire-and-forget).
    void play(SoundID id, float volumeScale = 1.f);

    // Play a sound for a rarity tier (maps to Reveal* automatically).
    void playRevealForRarity(int rarityInt); // pass static_cast<int>(rarity)

    // Master volume 0–100 applied to all sounds.
    void  setMasterVolume(float vol);
    float masterVolume()  const { return m_masterVolume; }

    // Mute toggle.
    void  setMuted(bool muted);
    bool  isMuted() const { return m_muted; }
    void  toggleMute();

private:
    AudioManager() = default;

    // We keep a small pool of sf::Sound objects to allow overlapping playback.
    static constexpr int POOL_SIZE = 8;

    sf::Sound& nextFreeSound();

    std::unordered_map<int, sf::SoundBuffer> m_buffers; // key = SoundID int
    std::array<sf::Sound, POOL_SIZE>         m_pool;
    int                                      m_poolIdx     { 0 };
    float                                    m_masterVolume { 80.f };
    bool                                     m_muted        { false };
    bool                                     m_loaded       { false };
};

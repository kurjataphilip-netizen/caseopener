#pragma once

#include "Case.hpp"
#include <vector>
#include <string>
#include <unordered_map>

// ── CaseRegistry ──────────────────────────────────────────────────────────────
// Singleton store of every Case in the game.
//
// Usage:
//   CaseRegistry::instance().load();                 // call once at startup
//   const Case* c = CaseRegistry::instance().find("standard");
//   auto item = c->open();
// ─────────────────────────────────────────────────────────────────────────────
class CaseRegistry
{
public:
    static CaseRegistry& instance();

    // Registers all built-in cases.  Safe to call multiple times (no-op after first).
    // ItemRegistry::instance().load() must be called first.
    void load();

    // Returns nullptr if id not found.
    const Case* find(const std::string& id) const;
          Case* find(const std::string& id);

    // All cases in registration order.
    const std::vector<Case>& all() const { return m_cases; }

    // Cases sorted cheapest → most expensive.
    std::vector<const Case*> sortedByPrice() const;

private:
    CaseRegistry() = default;

    void registerCase(Case c);

    std::vector<Case>                          m_cases;
    std::unordered_map<std::string, std::size_t> m_index;
    bool m_loaded { false };
};

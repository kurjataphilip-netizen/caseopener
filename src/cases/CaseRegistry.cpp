#include "CaseRegistry.hpp"
#include <algorithm>
#include <iostream>

// ── instance ──────────────────────────────────────────────────────────────────
CaseRegistry& CaseRegistry::instance()
{
    static CaseRegistry s_instance;
    return s_instance;
}

// ═════════════════════════════════════════════════════════════════════════════
// load  —  Define all 5 cases here.
//
// Price tier design:
//   $0.99  Starter Case     — flat global odds, beginner pool
//   $2.49  Field Case       — slight boost to MilSpec+
//   $4.99  Tactical Case    — noticeable boost to Restricted+
//   $9.99  Elite Case       — meaningful boost to Classified/Covert
//   $24.99 Contraband Case  — max rare boost, Contraband possible
//
// The log2(price) formula in Case::effectiveRarityWeight() automatically
// scales rare weights; the RarityOddsOverride below further fine-tunes
// each case's flavour beyond what price alone gives.
// ═════════════════════════════════════════════════════════════════════════════
void CaseRegistry::load()
{
    if (m_loaded) return;
    m_loaded = true;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. STARTER CASE  —  $0.99
    //    Pure global-odds case; good for new players / tutorial.
    // ─────────────────────────────────────────────────────────────────────────
    {
        Case c("starter", "Starter Case", "assets/images/case_starter.png", 0.99f);

        c.addItem("glock_sand_dune")
         .addItem("p2000_silver")
         .addItem("mp9_storm")
         .addItem("nova_gradan")
         .addItem("m4a4_radiation_hazard")
         .addItem("famas_mecha_industries")
         .addItem("hkp2000_corticera")
         .addItem("ak47_uncharted")
         .addItem("mp5sd_phosphor")
         .addItem("awp_containment_breach");

        // No odds override — uses raw global weights from Rarity.hpp
        registerCase(std::move(c));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. FIELD CASE  —  $2.49
    //    Cuts Consumer probability; MilSpec pool only (no Consumer/Industrial).
    // ─────────────────────────────────────────────────────────────────────────
    {
        Case c("field", "Field Case", "assets/images/case_field.png", 2.49f);

        // No Consumer/Industrial items in this pool — cleaner drops
        c.addItem("m4a4_radiation_hazard")
         .addItem("famas_mecha_industries")
         .addItem("hkp2000_corticera")
         .addItem("ak47_uncharted")
         .addItem("mp5sd_phosphor")
         .addItem("awp_containment_breach")
         .addItem("m4a1s_welcome_to_the_jungle")
         .addItem("ak47_redline");

        // Boost MilSpec and above; suppress lower tiers
        RarityOddsOverride odds;
        odds.weights[static_cast<int>(Rarity::Consumer)]   = 0.f;   // disabled
        odds.weights[static_cast<int>(Rarity::Industrial)]  = 0.f;   // disabled
        odds.weights[static_cast<int>(Rarity::MilSpec)]    = 60.f;
        odds.weights[static_cast<int>(Rarity::Restricted)] = 25.f;
        odds.weights[static_cast<int>(Rarity::Classified)] = 10.f;
        odds.weights[static_cast<int>(Rarity::Covert)]     = 4.5f;
        odds.weights[static_cast<int>(Rarity::Contraband)] = 0.f;   // not in pool
        c.setRarityOdds(odds);

        registerCase(std::move(c));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. TACTICAL CASE  —  $4.99
    //    Mid-tier case; Restricted is the new floor, Covert accessible.
    // ─────────────────────────────────────────────────────────────────────────
    {
        Case c("tactical", "Tactical Case", "assets/images/case_tactical.png", 4.99f);

        c.addItem("ak47_uncharted")
         .addItem("mp5sd_phosphor")
         .addItem("awp_containment_breach")
         .addItem("m4a1s_welcome_to_the_jungle")
         .addItem("ak47_redline")
         .addItem("m4a4_howl");

        RarityOddsOverride odds;
        odds.weights[static_cast<int>(Rarity::Consumer)]   = 0.f;
        odds.weights[static_cast<int>(Rarity::Industrial)]  = 0.f;
        odds.weights[static_cast<int>(Rarity::MilSpec)]    = 0.f;
        odds.weights[static_cast<int>(Rarity::Restricted)] = 55.f;
        odds.weights[static_cast<int>(Rarity::Classified)] = 30.f;
        odds.weights[static_cast<int>(Rarity::Covert)]     = 14.f;
        odds.weights[static_cast<int>(Rarity::Contraband)] = 0.f;
        c.setRarityOdds(odds);

        registerCase(std::move(c));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 4. ELITE CASE  —  $9.99
    //    High-end case; Classified is the floor, Covert pool is larger.
    //    Covert items get a higher per-entry weight.
    // ─────────────────────────────────────────────────────────────────────────
    {
        Case c("elite", "Elite Case", "assets/images/case_elite.png", 9.99f);

        c.addItem("awp_containment_breach")
         .addItem("m4a1s_welcome_to_the_jungle")
         .addItem("ak47_redline",  1.5f)   // slightly higher weight within Covert
         .addItem("m4a4_howl",     2.0f)   // most-wanted — boosted weight
         .addItem("karambit_fade", 0.5f);  // rare within Contraband tier

        RarityOddsOverride odds;
        odds.weights[static_cast<int>(Rarity::Consumer)]   = 0.f;
        odds.weights[static_cast<int>(Rarity::Industrial)]  = 0.f;
        odds.weights[static_cast<int>(Rarity::MilSpec)]    = 0.f;
        odds.weights[static_cast<int>(Rarity::Restricted)] = 0.f;
        odds.weights[static_cast<int>(Rarity::Classified)] = 50.f;
        odds.weights[static_cast<int>(Rarity::Covert)]     = 42.f;
        odds.weights[static_cast<int>(Rarity::Contraband)] = 0.f;  // priceBonus handles this
        c.setRarityOdds(odds);

        registerCase(std::move(c));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. CONTRABAND CASE  —  $24.99
    //    Prestige case; every tier represented, Contraband actually rollable,
    //    price bonus pushes odds dramatically toward the top end.
    // ─────────────────────────────────────────────────────────────────────────
    {
        Case c("contraband", "Contraband Case",
               "assets/images/case_contraband.png", 24.99f);

        // Full pool — all rarities represented
        c.addItem("glock_sand_dune")
         .addItem("p2000_silver")
         .addItem("mp9_storm")
         .addItem("m4a4_radiation_hazard")
         .addItem("famas_mecha_industries")
         .addItem("ak47_uncharted")
         .addItem("awp_containment_breach")
         .addItem("m4a1s_welcome_to_the_jungle")
         .addItem("ak47_redline")
         .addItem("m4a4_howl",     1.5f)
         .addItem("karambit_fade", 1.0f);  // the dream drop

        // Boosted rare weights — log2(price) in effectiveRarityWeight()
        // does most of the heavy lifting; these fine-tune the floor.
        RarityOddsOverride odds;
        odds.weights[static_cast<int>(Rarity::Consumer)]   = 20.f;
        odds.weights[static_cast<int>(Rarity::Industrial)]  = 15.f;
        odds.weights[static_cast<int>(Rarity::MilSpec)]    = 10.f;
        odds.weights[static_cast<int>(Rarity::Restricted)] = 5.f;
        odds.weights[static_cast<int>(Rarity::Classified)] = 3.f;
        odds.weights[static_cast<int>(Rarity::Covert)]     = 1.5f;
        odds.weights[static_cast<int>(Rarity::Contraband)] = 0.5f;  // real chance!
        c.setRarityOdds(odds);

        registerCase(std::move(c));
    }

    std::cout << "[CaseRegistry] Loaded " << m_cases.size() << " cases.\n";
}

// ── find (const) ──────────────────────────────────────────────────────────────
const Case* CaseRegistry::find(const std::string& id) const
{
    auto it = m_index.find(id);
    if (it == m_index.end()) return nullptr;
    return &m_cases[it->second];
}

// ── find (mutable) ────────────────────────────────────────────────────────────
Case* CaseRegistry::find(const std::string& id)
{
    auto it = m_index.find(id);
    if (it == m_index.end()) return nullptr;
    return &m_cases[it->second];
}

// ── sortedByPrice ─────────────────────────────────────────────────────────────
std::vector<const Case*> CaseRegistry::sortedByPrice() const
{
    std::vector<const Case*> ptrs;
    ptrs.reserve(m_cases.size());
    for (const auto& c : m_cases) ptrs.push_back(&c);
    std::sort(ptrs.begin(), ptrs.end(),
              [](const Case* a, const Case* b){ return a->price() < b->price(); });
    return ptrs;
}

// ── registerCase ──────────────────────────────────────────────────────────────
void CaseRegistry::registerCase(Case c)
{
    if (m_index.count(c.id()))
    {
        std::cerr << "[CaseRegistry] Duplicate case id: " << c.id() << " — skipped.\n";
        return;
    }
    m_index[c.id()] = m_cases.size();
    m_cases.push_back(std::move(c));
}

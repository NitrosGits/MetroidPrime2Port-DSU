// F1 overlay: The Cheats page, the Prime 1 port's debug_ui/cheats.cpp for this game: items,
// health, teleport and the room's layers. State and helpers shared with the other parts are in
// debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_warp.h"

#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CMemoryCard.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptObjects/CScriptConditionalRelay.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace PortDebug {

namespace {

// The Prime 1 port's Cheats page (debug_ui/cheats.cpp) for this game: the item list is Echoes'
// own and the grant path is the console's `give`/`take` (platform/port_console.cpp, which is
// CScriptPickup's own path): capacity and amount move together, and the step is a delta, so a
// slider sets a value rather than adding one. Zero removes the item, which is all it takes -
// every ability check here reads the capacity (HasPowerUp is `mCapacity > 0`).
void GrantItem(CPlayerState& ps, CPlayerState::EItemType type, int capacity, int amount) {
  // The Prime 1 port's GrantItem in this game's terms: `capacity` is what MP1 calls the power-up
  // and `amount` what it calls the pickup, and the two fields are named that way here. The
  // capacity is moved by the difference, because that is how AddPowerUp takes an expansion, and
  // the amount is then set outright - sliding a counter down must lower the amount even when the
  // capacity has room for it (which AddPowerUp alone would not do).
  ps.AddPowerUp(type, capacity - ps.GetItemCapacity(type));
  ps.SetItemAmount(type, amount);
  // A translator gate reads the inventory once, when its area loads; this has it read again.
  CScriptConditionalRelay::PortNoteInventoryCheat();
}

struct SItemCheck {
  const char* name;
  CPlayerState::EItemType type;
};

constexpr SItemCheck kAbilityItems[] = {
    {"Power Beam", CPlayerState::kIT_PowerBeam},
    {"Dark Beam", CPlayerState::kIT_DarkBeam},
    {"Light Beam", CPlayerState::kIT_LightBeam},
    {"Annihilator Beam", CPlayerState::kIT_AnnihilatorBeam},
    {"Charge Beam", CPlayerState::kIT_ChargeBeam},
    {"Super Missile", CPlayerState::kIT_SuperMissile},
    {"Darkburst", CPlayerState::kIT_Darkburst},
    {"Sunburst", CPlayerState::kIT_Sunburst},
    {"Sonic Boom", CPlayerState::kIT_SonicBoom},
    {"Combat Visor", CPlayerState::kIT_CombatVisor},
    {"Scan Visor", CPlayerState::kIT_ScanVisor},
    {"Dark Visor", CPlayerState::kIT_DarkVisor},
    {"Echo Visor", CPlayerState::kIT_EchoVisor},
    {"Varia Suit", CPlayerState::kIT_VariaSuit},
    {"Dark Suit", CPlayerState::kIT_DarkSuit},
    {"Light Suit", CPlayerState::kIT_LightSuit},
    {"Morph Ball", CPlayerState::kIT_MorphBall},
    {"Boost Ball", CPlayerState::kIT_BoostBall},
    {"Spider Ball", CPlayerState::kIT_SpiderBall},
    {"Morph Ball Bombs", CPlayerState::kIT_MorphBallBombs},
    {"Grapple Beam", CPlayerState::kIT_GrappleBeam},
    {"Space Jump Boots", CPlayerState::kIT_SpaceJumpBoots},
    {"Gravity Boost", CPlayerState::kIT_GravityBoost},
    {"Seeker Launcher", CPlayerState::kIT_SeekerLauncher},
    {"Screw Attack", CPlayerState::kIT_ScrewAttack},
    {"Violet Translator", CPlayerState::kIT_VioletTranslator},
    {"Amber Translator", CPlayerState::kIT_AmberTranslator},
    {"Emerald Translator", CPlayerState::kIT_EmeraldTranslator},
    {"Cobalt Translator", CPlayerState::kIT_CobaltTranslator},
};

constexpr SItemCheck kKeyItems[] = {
    {"Temple Key 1", CPlayerState::kIT_TempleKey1},
    {"Temple Key 2", CPlayerState::kIT_TempleKey2},
    {"Temple Key 3", CPlayerState::kIT_TempleKey3},
    {"Agon Key 1", CPlayerState::kIT_AgonKey1},
    {"Agon Key 2", CPlayerState::kIT_AgonKey2},
    {"Agon Key 3", CPlayerState::kIT_AgonKey3},
    {"Torvus Key 1", CPlayerState::kIT_TorvusKey1},
    {"Torvus Key 2", CPlayerState::kIT_TorvusKey2},
    {"Torvus Key 3", CPlayerState::kIT_TorvusKey3},
    {"Hive Key 1", CPlayerState::kIT_HiveKey1},
    {"Hive Key 2", CPlayerState::kIT_HiveKey2},
    {"Hive Key 3", CPlayerState::kIT_HiveKey3},
};

// The toggles as a grid, as many columns as the widest name leaves room for.
void DrawItemChecks(CPlayerState& ps, const char* tableId, const SItemCheck* items, int count) {
  float nameWidth = 0.f;
  for (int i = 0; i < count; ++i) {
    nameWidth = std::max(nameWidth, ImGui::CalcTextSize(items[i].name).x);
  }
  nameWidth += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x +
               ImGui::GetStyle().ItemSpacing.x * 2.f;
  const int columns = std::clamp(int(ImGui::GetContentRegionAvail().x / nameWidth), 1, 4);
  if (ImGui::BeginTable(tableId, columns)) {
    for (int i = 0; i < count; ++i) {
      ImGui::TableNextColumn();
      bool owned = ps.HasPowerUp(items[i].type);
      if (ImGui::Checkbox(items[i].name, &owned)) {
        GrantItem(ps, items[i].type, owned ? 1 : 0, owned ? 1 : 0);
      }
    }
    ImGui::EndTable();
  }
}

} // namespace

void DrawCheatsTab() {
  ImGui::Checkbox("God mode", &sGodMode);
  ItemHelp("The player takes no damage; other actors still do. Not saved.");

  CPlayerState* ps = gpGameState != nullptr ? gpGameState->GetPlayerState().GetPtr() : nullptr;
  // The front end has its own player state and its own Samus; the page is about the game's.
  if (ps == nullptr ||
      (gpGameState != nullptr && gpGameState->GetGameMode().GetGameModeType() == 'FRND')) {
    ImGui::TextDisabled("Waiting for gameplay...");
    return;
  }
  CHealthInfo* health = ps->HealthInfo();
  ImGui::Text("Health: %.0f / %.0f", health->GetHP(), ps->CalculateHealth());
  if (ImGui::Button("Full health")) {
    health->SetHP(ps->CalculateHealth());
  }
  ImGui::SameLine();
  if (ImGui::Button("Grant everything")) {
    for (const SItemCheck& item : kAbilityItems) {
      GrantItem(*ps, item.type, 1, 1);
    }
    for (const SItemCheck& item : kKeyItems) {
      GrantItem(*ps, item.type, 1, 1);
    }
    for (const CPlayerState::EItemType counter :
         {CPlayerState::kIT_Missile, CPlayerState::kIT_Powerbomb, CPlayerState::kIT_EnergyTanks,
          CPlayerState::kIT_DarkAmmo, CPlayerState::kIT_LightAmmo}) {
      const int max = CPlayerState::GetPowerUpMaxValue(counter);
      GrantItem(*ps, counter, max, max);
    }
    health->SetHP(ps->CalculateHealth());
  }

  ImGui::SeparatorText("Abilities");
  DrawItemChecks(*ps, "##abilitychecks", kAbilityItems, int(std::size(kAbilityItems)));
  if (ImGui::CollapsingHeader("Keys")) {
    DrawItemChecks(*ps, "##keychecks", kKeyItems, int(std::size(kKeyItems)));
  }

  const auto counter = [ps](const char* label, CPlayerState::EItemType type) {
    const int max = CPlayerState::GetPowerUpMaxValue(type);
    int amount = std::min(ps->GetItemAmount(type, false), max);
    // A fixed width, so the label sits next to its slider instead of at the page's right edge.
    ImGui::SetNextItemWidth(220.f);
    if (ImGui::SliderInt(label, &amount, 0, max)) {
      // The capacity stays at its maximum, as with the game's own expansions, and only the
      // amount moves: a slide down must take ammo away even though the capacity has room.
      GrantItem(*ps, type, max, amount);
    }
  };
  counter("Missiles", CPlayerState::kIT_Missile);
  counter("Power Bombs", CPlayerState::kIT_Powerbomb);
  counter("Dark Ammo", CPlayerState::kIT_DarkAmmo);
  counter("Light Ammo", CPlayerState::kIT_LightAmmo);
  // Energy tanks are the one counter that also moves the health bar.
  {
    const int max = CPlayerState::GetPowerUpMaxValue(CPlayerState::kIT_EnergyTanks);
    int tanks = std::min(ps->GetItemAmount(CPlayerState::kIT_EnergyTanks, false), max);
    ImGui::SetNextItemWidth(220.f);
    if (ImGui::SliderInt("Energy Tanks", &tanks, 0, max)) {
      GrantItem(*ps, CPlayerState::kIT_EnergyTanks, max, tanks);
      health->SetHP(ps->CalculateHealth());
    }
  }
  ItemHelp("The items go into the save in memory: the next save on the memory card writes them, "
           "and nothing here is kept across a restart on its own.");
  ImGui::SeparatorText("Teleport");
  if (ImGui::Button("Previous area")) {
    PortWarp::CycleArea(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next area")) {
    PortWarp::CycleArea(1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Previous world")) {
    PortWarp::CycleWorld(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next world")) {
    PortWarp::CycleWorld(1);
  }
  // The Prime 1 port's Teleport block (debug_ui/cheats.cpp): a filtered list of the current
  // world's rooms and the save's world table, both by name. The requests are the same PortWarp
  // ones the console's `warp`/`world` verbs and MP2_TELEPORT use, so nothing here walks around
  // what the game already does.
  if (sRooms.empty()) {
    ImGui::TextDisabled("(no world yet)");
  } else {
    const int areaCount = static_cast< int >(sRooms.size());
    const int current = sAreaId;
    // Between two updates the published ids are the same frame's, so this only guards a world
    // that is arriving: an area index outside the table is not looked up.
    const bool currentValid = current >= 0 && current < areaCount;
    const auto roomLabel = [](int index) { return sRooms[index].name; };
    const auto roomExists = [](int index) { return sRooms[index].real; };
    const auto goToRoom = [](int index) {
      char text[16];
      std::snprintf(text, sizeof(text), "%d", index);
      PortWarp::Area(text);
    };

    static char sRoomFilter[64] = {};
    const float goWidth = ImGui::CalcTextSize("Reload").x + ImGui::GetStyle().FramePadding.x * 2.f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth -
                            ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::BeginCombo("##room", currentValid ? roomLabel(current).c_str() : "(no area)",
                          ImGuiComboFlags_HeightLarge)) {
#if !defined(__ANDROID__) // it would raise the on-screen keyboard every time
      if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
      }
#endif
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::InputTextWithHint("##roomfilter", "Filter rooms", sRoomFilter, sizeof(sRoomFilter));
      for (int i = 0; i < areaCount; ++i) {
        if (!roomExists(i)) {
          continue;
        }
        const std::string label = roomLabel(i);
        if (sRoomFilter[0] != '\0' && SDL_strcasestr(label.c_str(), sRoomFilter) == nullptr &&
            SDL_strcasestr(sRooms[i].label.c_str(), sRoomFilter) == nullptr) {
          continue;
        }
        ImGui::PushID(i);
        if (ImGui::Selectable(label.c_str(), i == current)) {
          goToRoom(i);
        }
        ImGui::SetItemTooltip("%s", sRooms[i].label.c_str());
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Pick a room to go there. A room that is not loaded streams in first.");
    ImGui::SameLine();
    ImGui::BeginDisabled(!currentValid);
    if (ImGui::Button("Reload")) {
      // The area warp ignores a request for the room it is already in, so a reload is the
      // world warp the elevators use, to the same world and room.
      char worldText[16];
      char areaText[16];
      std::snprintf(worldText, sizeof(worldText), "%08x", sWorldId);
      std::snprintf(areaText, sizeof(areaText), "%d", current);
      PortWarp::World(worldText, areaText);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Rebuild the room from the disc, as leaving and coming back does.");

    // Rebuilt when the card's table changes size, so loading another save does not leave the
    // combo showing the previous one's worlds.
    static size_t sWorldListCount = 0;
    static std::vector< std::pair< uint32_t, std::string > > sWorldList;
    const int worldCount =
        gpMemoryCard != nullptr ? static_cast< int >(gpMemoryCard->GetMemoryWorlds().size()) : 0;
    if (worldCount > 0 && static_cast< size_t >(worldCount) != sWorldListCount) {
      sWorldListCount = static_cast< size_t >(worldCount);
      sWorldList.clear();
      const rstl::vector< CMemoryCard::MemoryWorld >& worlds = gpMemoryCard->GetMemoryWorlds();
      for (int i = 0; i < worlds.size(); ++i) {
        const uint32_t id = static_cast< uint32_t >(worlds[i].first);
        std::string name = PlainWorldName(worlds[i].second.GetFrontEndName());
        if (name.empty()) {
          char buf[32];
          std::snprintf(buf, sizeof(buf), "MLVL %08X", static_cast< unsigned >(id));
          name = buf;
        }
        sWorldList.emplace_back(id, name);
      }
    }
    if (sWorldList.empty()) {
      ImGui::TextDisabled("(the memory card's world table is not loaded yet)");
    } else {
      const char* currentName = "(another world)";
      for (const std::pair< uint32_t, std::string >& entry : sWorldList) {
        if (entry.first == sWorldId) {
          currentName = entry.second.c_str();
        }
      }
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (ImGui::BeginCombo("##world", currentName)) {
        for (const std::pair< uint32_t, std::string >& entry : sWorldList) {
          ImGui::PushID(static_cast< int >(entry.first));
          if (ImGui::Selectable(entry.second.c_str(), entry.first == sWorldId)) {
            char text[16];
            std::snprintf(text, sizeof(text), "%08x", entry.first);
            PortWarp::World(text, "");
          }
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }
      ImGui::SetItemTooltip("Pick a world to reload into its first room.");
    }
  }

  ImGui::SeparatorText("Room layers");
  ImGui::TextDisabled("The room's script layers, as the world's layer state has them.");
  ItemHelp("The game switches them for good as the story moves on (a boss fight turns its "
           "first-pass layer off when it starts), and the next save keeps them, so leaving a fight "
           "by a warp can leave its room without the enemy. Turning a layer on loads it now; "
           "turning one off takes effect when the room loads again (Reload).");
  if (sLayers.empty() || sLayersArea != sAreaId) {
    ImGui::TextDisabled("(no layer state for this room)");
  } else {
    for (int i = 0; i < static_cast< int >(sLayers.size()); ++i) {
      bool active = sLayers[i].active;
      char label[96];
      std::snprintf(label, sizeof(label), "%d: %s##layer%d", i, sLayers[i].name.c_str(), i);
      if (ImGui::Checkbox(label, &active)) {
        if (active) {
          PortWarp::Layer(i);
        } else {
          PortWarp::LayerOff(i);
        }
      }
    }
  }
}

} // namespace PortDebug

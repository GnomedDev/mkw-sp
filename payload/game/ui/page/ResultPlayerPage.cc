#include "ResultPlayerPage.hh"

#include "game/ui/SectionManager.hh"
#include "game/system/RaceConfig.hh"

namespace UI {

PageId ResultPlayerPage::getReplacement() {
    auto currentSectionId = SectionManager::Instance()->currentSection()->id();
    return currentSectionId == SectionId::GP ? PageId::AfterGpMenu : PageId::AfterVsMenu;
}

PageId ResultRaceUpdatePage::getReplacement() {
    const auto &raceScenario = System::RaceConfig::Instance()->raceScenario();
    return raceScenario.spMaxTeamSize < 2 ? PageId::ResultRaceTotal : PageId::ResultTeamVSTotal;
}

} // namespace UI

#include "main.hpp"

#include "GlobalNamespace/NoteCutCoreEffectsSpawner.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/NoteCutHapticEffect.hpp"

#include "GlobalNamespace/OculusAdvancedHapticFeedbackPlayer.hpp"

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
// Stores the ID and version of our mod, and is sent to
// the modloader upon startup

DECLARE_CONFIG(BetterHapticsConfig) {
    CONFIG_VALUE(globalBypass, bool, "globalBypass", false);
    CONFIG_VALUE(separateArcs, bool, "separateArcs", true);
};

bool get_global_bypass() {
    return getBetterHapticsConfig().globalBypass.GetValue();
}

void set_global_bypass(bool globalBypass) {
    getBetterHapticsConfig().globalBypass.SetValue(globalBypass);
}

bool get_separate_arcs() {
    return getBetterHapticsConfig().separateArcs.GetValue();
}

void set_separate_arcs(bool separateArcs) {
    getBetterHapticsConfig().separateArcs.SetValue(separateArcs);
}

// Called at the early stages of game loading
MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
    *info = modInfo.to_c();

    // File logging
    Paper::Logger::RegisterFileContextId(PaperLogger.tag);

    PaperLogger.info("Completed setup!");
}

MAKE_HOOK_MATCH(
    NoteCutCoreEffectsSpawner_PlayHitNoteHapticEffect,
    &GlobalNamespace::NoteCutCoreEffectsSpawner::PlayHitNoteHapticEffect, void,
    GlobalNamespace::NoteCutCoreEffectsSpawner *self, GlobalNamespace::NoteCutInfo noteCutInfo, GlobalNamespace::NoteData *noteData
) {
    if (get_global_bypass() || get_separate_arcs()) {
        NoteCutCoreEffectsSpawner_PlayHitNoteHapticEffect(self, noteCutInfo, noteData);
        return;
    }

    if (!noteCutInfo.allIsOK) {
        self->_noteCutHapticEffect->HitNote(noteCutInfo.saberType, GlobalNamespace::NoteCutHapticEffect::Type::BadCut);
    } else {
        self->_noteCutHapticEffect->HitNote(noteCutInfo.saberType, GlobalNamespace::NoteCutHapticEffect::Type::Normal);
    }
}

MAKE_HOOK_MATCH(
    NoteCutCoreEffectsSpawner_PlayHitChainNoteHapticEffect,
    &GlobalNamespace::NoteCutCoreEffectsSpawner::PlayHitChainNoteHapticEffect, void,
    GlobalNamespace::NoteCutCoreEffectsSpawner *self, GlobalNamespace::NoteCutInfo noteCutInfo, bool isChainHead
) {
    if (get_global_bypass()) {
        NoteCutCoreEffectsSpawner_PlayHitChainNoteHapticEffect(self, noteCutInfo, isChainHead);
        return;
    }

    if (!noteCutInfo.allIsOK) {
        self->_noteCutHapticEffect->HitNote(noteCutInfo.saberType, GlobalNamespace::NoteCutHapticEffect::Type::BadCut);
    } else if (isChainHead) {
        self->_noteCutHapticEffect->HitNote(noteCutInfo.saberType, GlobalNamespace::NoteCutHapticEffect::Type::ChainHead);
    } else {
        self->_noteCutHapticEffect->HitNote(noteCutInfo.saberType, GlobalNamespace::NoteCutHapticEffect::Type::ChainLink);
    }
}

MAKE_HOOK_MATCH(
    OculusAdvancedHapticFeedbackPlayer_CanPlayHapticPreset,
    &GlobalNamespace::OculusAdvancedHapticFeedbackPlayer::CanPlayHapticPreset, bool,
    GlobalNamespace::OculusAdvancedHapticFeedbackPlayer *self, Libraries::HM::HMLib::VR::HapticPresetSO *hapticPreset, UnityEngine::XR::XRNode node
) {
    if (get_global_bypass()) {
        return OculusAdvancedHapticFeedbackPlayer_CanPlayHapticPreset(self, hapticPreset, node);
    }

    return false;
}

float calculate_offset_y(UnityEngine::Component *component) {
    UnityW<UnityEngine::Transform> transform = component->transform;
    UnityW<UnityEngine::RectTransform> rectTransform = transform.cast<UnityEngine::RectTransform>();
    UnityEngine::Rect rect = rectTransform->rect;

    return -rect.m_Height;
}

void menu_controller(HMUI::ViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (!firstActivation) {
        return;
    }

    float offsetY = 0.0f;

    BSML::ToggleSetting *globalBypassToggle = BSML::Lite::CreateToggle(self->transform, "Global Bypass", get_global_bypass(), {0.0f, offsetY}, set_global_bypass);
    offsetY += calculate_offset_y(globalBypassToggle);

    BSML::ToggleSetting *separateArcsToggle = BSML::Lite::CreateToggle(self->transform, "Separate Arcs", get_separate_arcs(), {0.0f, offsetY}, set_separate_arcs);
    offsetY += calculate_offset_y(separateArcsToggle);
}

// Called later on in the game loading - a good time to install function hooks
MOD_EXTERN_FUNC void late_load() noexcept {
    il2cpp_functions::Init();

    getBetterHapticsConfig().Init(modInfo);
    BSML::Init();

    BSML::Register::RegisterMainMenuViewControllerMethod(
        "BetterHaptics by Crunkle",
        "BetterHaptics",
        "",
        menu_controller
    );

    PaperLogger.info("Installing hooks...");

    INSTALL_HOOK(PaperLogger, NoteCutCoreEffectsSpawner_PlayHitNoteHapticEffect);
    INSTALL_HOOK(PaperLogger, NoteCutCoreEffectsSpawner_PlayHitChainNoteHapticEffect);
    INSTALL_HOOK(PaperLogger, OculusAdvancedHapticFeedbackPlayer_CanPlayHapticPreset);

    PaperLogger.info("Installed all hooks!");
}

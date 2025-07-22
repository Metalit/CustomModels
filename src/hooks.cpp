#include "hooks.hpp"

#include "GlobalNamespace/ColorsOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/ConditionalMaterialSwitcher.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/LayerMasks.hpp"
#include "GlobalNamespace/LevelListTableCell.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/NoteDebrisSpawner.hpp"
#include "GlobalNamespace/ObstacleMaterialSetter.hpp"
#include "GlobalNamespace/SaberTrailRenderer.hpp"
#include "GlobalNamespace/TutorialSceneSetupData.hpp"
#include "GlobalNamespace/TutorialScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/UIKeyboardManager.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"
#include "Zenject/DiContainer.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "metacore/shared/operators.hpp"
#include "note.hpp"
#include "pointers.hpp"
#include "registration.hpp"
#include "trail.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

// use the PC material to fix trails cutting into each other
MAKE_AUTO_HOOK_MATCH(ConditionalMaterialSwitcher_Awake, &ConditionalMaterialSwitcher::Awake, void, ConditionalMaterialSwitcher* self) {
    if (!getConfig().Enabled.GetValue() || self->name != "Trail(Clone)")
        ConditionalMaterialSwitcher_Awake(self);
}

// don't run the base class Awake on our saber trail
MAKE_AUTO_HOOK_MATCH(SaberTrail_Awake, &SaberTrail::Awake, void, SaberTrail* self) {
    if (!il2cpp_utils::try_cast<CustomModels::CustomSaberTrail>(self).has_value())
        SaberTrail_Awake(self);
}

// call PreLateUpdate to add movement data for our saber trail
MAKE_AUTO_HOOK_MATCH(SaberTrail_LateUpdate, &SaberTrail::LateUpdate, void, SaberTrail* self) {
    if (auto custom = il2cpp_utils::try_cast<CustomModels::CustomSaberTrail>(self)) {
        if (!(*custom)->PreLateUpdate())
            return;
    }
    SaberTrail_LateUpdate(self);
}

// slightly crop saber trail edges
MAKE_AUTO_HOOK_MATCH(
    SaberTrailRenderer_UpdateVertices,
    &SaberTrailRenderer::UpdateVertices,
    void,
    SaberTrailRenderer* self,
    TrailElementCollection* elements,
    Color color
) {
    SaberTrailRenderer_UpdateVertices(self, elements, color);

    for (int i = 0; i < self->_granularity * 3; i += 3) {
        if (self->_uvs[i].y <= 0.98)
            continue;
        self->_uvs[i].y = 0.98;
        self->_uvs[i + 1].y = 0.98;
        self->_uvs[i + 2].y = 0.98;
    }
}

static void Scale(UnityEngine::Component* object, float size) {
    auto transform = object->transform;
    transform->localScale = transform->localScale * size;
}

// apply note size
MAKE_AUTO_HOOK_MATCH(
    NoteControllerInit,
    &NoteController::Init,
    void,
    NoteController* self,
    NoteData* noteData,
    ByRef<NoteSpawnData> noteSpawnData,
    float endRotation,
    float uniformScale,
    bool rotatesTowardsPlayer,
    bool useRandomRotation
) {
    float size = 1;
    if (getConfig().Enabled.GetValue())
        size = getConfig().NotesSettings().Size();

    NoteControllerInit(self, noteData, noteSpawnData, endRotation, uniformScale * size, rotatesTowardsPlayer, useRandomRotation);

    if (size == 1)
        return;
    float inverse = 1 / size;

    if (auto note = il2cpp_utils::try_cast<GameNoteController>(self).value_or(nullptr)) {
        for (auto hitbox : note->_bigCuttableBySaberList)
            Scale(hitbox, inverse);
        for (auto hitbox : note->_smallCuttableBySaberList)
            Scale(hitbox, inverse);
    } else if (auto bomb = il2cpp_utils::try_cast<BombNoteController>(self).value_or(nullptr))
        Scale(bomb->_cuttableBySaber, inverse);
}

// initialize custom debris
static NoteDebris* lastDebris0;
static NoteDebris* lastDebris1;

MAKE_AUTO_HOOK_MATCH(
    NoteDebrisSpawner_SpawnNoteDebris,
    &NoteDebrisSpawner::SpawnNoteDebris,
    void,
    NoteDebrisSpawner* self,
    NoteData::GameplayType noteGameplayType,
    ByRef<NoteDebris*> debris0,
    ByRef<NoteDebris*> debris1
) {
    NoteDebrisSpawner_SpawnNoteDebris(self, noteGameplayType, debris0, debris1);
    lastDebris0 = *debris0;
    lastDebris1 = *debris1;
}

MAKE_AUTO_HOOK_MATCH(
    NoteDebrisSpawner_SpawnDebris,
    &NoteDebrisSpawner::SpawnDebris,
    void,
    NoteDebrisSpawner* self,
    NoteData_GameplayType noteGameplayType,
    Vector3 cutPoint,
    Vector3 cutNormal,
    float saberSpeed,
    Vector3 saberDir,
    Vector3 notePos,
    Quaternion noteRotation,
    Vector3 noteScale,
    ColorType colorType,
    float timeToNextColorNote,
    Vector3 moveVec
) {
    NoteDebrisSpawner_SpawnDebris(
        self, noteGameplayType, cutPoint, cutNormal, saberSpeed, saberDir, notePos, noteRotation, noteScale, colorType, timeToNextColorNote, moveVec
    );
    if (!lastDebris0 || !lastDebris1)
        return;
    if (auto visuals = lastDebris0->GetComponentInChildren<CustomModels::DebrisVisuals*>())
        visuals->ShowDebris(colorType, cutPoint, -cutNormal);
    if (auto visuals = lastDebris1->GetComponentInChildren<CustomModels::DebrisVisuals*>())
        visuals->ShowDebris(colorType, cutPoint, cutNormal);
}

// selectively disable material setting for obstacle core
MAKE_AUTO_HOOK_MATCH(
    ObstacleMaterialSetter_SetCoreMaterial,
    &ObstacleMaterialSetter::SetCoreMaterial,
    void,
    ObstacleMaterialSetter* self,
    UnityEngine::Renderer* renderer,
    BeatSaber::Settings::QualitySettings::ObstacleQuality obstacleQuality,
    bool screenDisplacementEffects
) {
    if (self->_lwCoreMaterial && self->_texturedCoreMaterial && self->_hwCoreMaterial)
        ObstacleMaterialSetter_SetCoreMaterial(self, renderer, obstacleQuality, screenDisplacementEffects);
}

// get color scheme and modifiers
MAKE_AUTO_HOOK_MATCH(
    GameplayCoreSceneSetupData_ctor,
    &GameplayCoreSceneSetupData::_ctor,
    void,
    GameplayCoreSceneSetupData* self,
    ByRef<BeatmapKey> p0,
    BeatmapLevel* p1,
    GameplayModifiers* modifiers,
    PlayerSpecificSettings* p3,
    PracticeSettings* p4,
    bool p5,
    EnvironmentInfoSO* p6,
    EnvironmentInfoSO* p7,
    ColorScheme* colorScheme,
    SettingsManager* p9,
    AudioClipAsyncLoader* p10,
    BeatmapDataLoader* p11,
    BeatmapLevelsEntitlementModel* p12,
    bool p13,
    bool p14,
    EnvironmentsListModel* p15,
    System::Nullable_1<RecordingToolManager::SetupData> p16
) {
    GameplayCoreSceneSetupData_ctor(self, p0, p1, modifiers, p3, p4, p5, p6, p7, colorScheme, p9, p10, p11, p12, p13, p14, p15, p16);

    CustomModels::colorScheme = colorScheme;
    CustomModels::modifiers = modifiers;
}

// not sure why hooking TutorialSceneSetupData_ctor doesn't work, maybe too small somehow?
MAKE_AUTO_HOOK_MATCH(
    TutorialScenesTransitionSetupDataSO_Init,
    &TutorialScenesTransitionSetupDataSO::Init,
    void,
    TutorialScenesTransitionSetupDataSO* self,
    PlayerSpecificSettings* settings
) {
    TutorialScenesTransitionSetupDataSO_Init(self, settings);

    auto setupData = (TutorialSceneSetupData*) self->_sceneSetupDataArray[2];
    CustomModels::colorScheme = setupData->colorScheme;
    CustomModels::modifiers = nullptr;
}

// enable menu pointers
MAKE_AUTO_HOOK_MATCH(
    MainFlowCoordinator_DidActivate,
    &MainFlowCoordinator::DidActivate,
    void,
    MainFlowCoordinator* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (getConfig().Enabled.GetValue())
        CustomModels::EnableMenuPointers();
}

// update menu pointer colors
MAKE_AUTO_HOOK_MATCH(
    ColorsOverrideSettingsPanelController_HandleOverrideColorsToggleValueChanged,
    &ColorsOverrideSettingsPanelController::HandleOverrideColorsToggleValueChanged,
    void,
    ColorsOverrideSettingsPanelController* self,
    bool isOn
) {
    ColorsOverrideSettingsPanelController_HandleOverrideColorsToggleValueChanged(self, isOn);
    CustomModels::UpdateMenuPointersColor();
}

MAKE_AUTO_HOOK_MATCH(
    ColorsOverrideSettingsPanelController_HandleEditColorSchemeControllerDidChangeColorScheme,
    &ColorsOverrideSettingsPanelController::HandleEditColorSchemeControllerDidChangeColorScheme,
    void,
    ColorsOverrideSettingsPanelController* self,
    ColorScheme* colorScheme
) {
    ColorsOverrideSettingsPanelController_HandleEditColorSchemeControllerDidChangeColorScheme(self, colorScheme);
    CustomModels::UpdateMenuPointersColor();
}

MAKE_AUTO_HOOK_MATCH(
    ColorsOverrideSettingsPanelController_HandleDropDownDidSelectCellWithIdx,
    &ColorsOverrideSettingsPanelController::HandleDropDownDidSelectCellWithIdx,
    void,
    ColorsOverrideSettingsPanelController* self,
    HMUI::DropdownWithTableView* dropDownWithTableView,
    int idx
) {
    ColorsOverrideSettingsPanelController_HandleDropDownDidSelectCellWithIdx(self, dropDownWithTableView, idx);
    CustomModels::UpdateMenuPointersColor();
}

// fix menu pointers with colliders
MAKE_AUTO_HOOK_MATCH(
    VRGraphicRaycaster_Raycast,
    &VRUIControls::VRGraphicRaycaster::Raycast,
    void,
    VRUIControls::VRGraphicRaycaster* self,
    UnityEngine::EventSystems::PointerEventData* eventData,
    System::Collections::Generic::List_1<UnityEngine::EventSystems::RaycastResult>* resultAppendList
) {
    auto mask = self->_blockingMask;
    self->_blockingMask = mask.m_Mask & ~LayerMasks::getStaticF_saberLayerMask().m_Mask;
    VRGraphicRaycaster_Raycast(self, eventData, resultAppendList);
    self->_blockingMask = mask;
}

// fix profile rename keyboard being behind the blocker
MAKE_AUTO_HOOK_MATCH(
    UIKeyboardManager_OpenKeyboardFor, &UIKeyboardManager::OpenKeyboardFor, void, UIKeyboardManager* self, HMUI::InputFieldView* input
) {
    if (auto inputModal = input->GetComponentInParent<HMUI::ModalView*>()) {
        auto inputModalCanvas = inputModal->GetComponent<UnityEngine::Canvas*>();
        auto keyboardModalCanvas = self->_keyboardModalView->GetComponent<UnityEngine::Canvas*>();
        keyboardModalCanvas->sortingOrder = inputModalCanvas->sortingOrder;
    }

    UIKeyboardManager_OpenKeyboardFor(self, input);
}

// only show delete button on hovered models
MAKE_AUTO_HOOK_MATCH(
    LevelListTableCell_RefreshVisuals, &LevelListTableCell::RefreshVisuals, void, LevelListTableCell* self
) {
    if (self->name->Contains("CustomModels"))
        self->_favoritesBadgeImage->enabled = self->highlighted;

    LevelListTableCell_RefreshVisuals(self);
}

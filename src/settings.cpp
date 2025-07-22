#include "settings.hpp"

#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "HMUI/ScrollView.hpp"
#include "UnityEngine/UI/LayoutRebuilder.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "config.hpp"
#include "loading.hpp"
#include "main.hpp"
#include "metacore/shared/ui.hpp"
#include "note.hpp"
#include "pointers.hpp"
#include "saber.hpp"
#include "selection.hpp"
#include "utils.hpp"
#include "wall.hpp"
#include "zip.hpp"

DEFINE_TYPE(CustomModels, SettingsCoordinator);
DEFINE_TYPE(CustomModels, ModSettings);
DEFINE_TYPE(CustomModels, SelectionSettings);
DEFINE_TYPE(CustomModels, PreviewSettings);

using namespace CustomModels;

SettingsCoordinator* SettingsCoordinator::instance = nullptr;
ModSettings* ModSettings::instance = nullptr;
SelectionSettings* SelectionSettings::instance = nullptr;
PreviewSettings* PreviewSettings::instance = nullptr;

template <class T>
static inline T* LazyCreate(T*& instance, char const* name) {
    if (!instance) {
        instance = BSML::Helpers::CreateViewController<T*>();
        instance->name = name;
    }
    return instance;
}

static int NullCheck(std::vector<void*> objects) {
    for (int i = 0; i < objects.size(); i++) {
        if (!objects[i])
            return i;
    }
    return -1;
}

#ifdef HOT_RELOAD
#define NULL_CHECK(...)                                     \
    if (int i = NullCheck({__VA_ARGS__}); i >= 0) {         \
        logger.debug("null required field at index {}", i); \
        return;                                             \
    }
#else
#define NULL_CHECK(...)
#endif

void SettingsCoordinator::DidActivate(bool firstActivation, bool, bool) {
    DisableMenuPointers();
    if (!firstActivation)
        return;
    showBackButton = true;
    SetTitle("Custom Models", HMUI::ViewController::AnimationType::None);
    ProvideInitialViewControllers(SelectionSettings::GetInstance(), ModSettings::GetInstance(), PreviewSettings::GetInstance(), nullptr, nullptr);
}

void SettingsCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController) {
    _parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
    EnableMenuPointers();
}

void SettingsCoordinator::OnDestroy() {
    instance = nullptr;
}

void SettingsCoordinator::Present() {
    logger.info("presenting settings");
    BSML::Helpers::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf()->PresentFlowCoordinator(
        GetInstance(), nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false
    );
}

SettingsCoordinator* SettingsCoordinator::GetInstance() {
    if (!instance)
        instance = BSML::Helpers::CreateFlowCoordinator<SettingsCoordinator*>();
    return instance;
}

int SettingsCoordinator::ModelSelection() {
    auto settings = GetInstance();
    switch (settings->modelType) {
        case 0:
            if (settings->trail)
                return settings->menuPointer ? (int) Selection::MenuTrail : (int) Selection::Trail;
            else
                return settings->menuPointer ? (int) Selection::MenuSaber : (int) Selection::Saber;
        case 1:
            return (int) Selection::Note;
        default:
            return (int) Selection::Wall;
    }
}

float SelectionSettings::CellSize() {
    return 8.5;
}

int SelectionSettings::NumberOfCells() {
    return models.size();
}

static GlobalNamespace::LevelListTableCell* CreateTableCellPrefab() {
    auto levelCollectionController = BSML::Helpers::GetDiContainer()->Resolve<GlobalNamespace::LevelCollectionViewController*>();
    auto base = levelCollectionController->transform->Find("LevelsTableView/TableView/Viewport/Content/LevelListTableCell");
    auto instance = UnityEngine::Object::Instantiate(base)->GetComponent<GlobalNamespace::LevelListTableCell*>();
    instance->name = "CustomModelsTableCell";

    UnityEngine::Object::DestroyImmediate(instance->_songBpmText->gameObject);
    UnityEngine::Object::DestroyImmediate(instance->_songDurationText->gameObject);
    UnityEngine::Object::DestroyImmediate(instance->_promoBadgeGo);
    UnityEngine::Object::DestroyImmediate(instance->_updatedBadgeGo);
    UnityEngine::Object::DestroyImmediate(instance->_favoritesBadgeImage->gameObject);
    UnityEngine::Object::DestroyImmediate(instance->transform->Find("BpmIcon")->gameObject);

    instance->_songNameText->rectTransform->anchorMax = {1.05, 0.5};
    instance->_songAuthorText->rectTransform->anchorMax = {1.05, 0.5};

    auto image = BSML::Lite::CreateClickableImage(instance, PNG_SPRITE(icons::delete), nullptr, {36, 0}, {5, 5});
    image->defaultColor = {1, 1, 1, 1};
    image->highlightColor = {1, 0.2, 0.2, 1};
    // closest field available
    instance->_favoritesBadgeImage = image;

    return instance;
}

HMUI::TableCell* SelectionSettings::CellForIdx(HMUI::TableView*, int idx) {
    if (!tableView)
        return nullptr;

    auto ret = tableView->DequeueReusableCellForIdentifier(reuseIdentifier);
    if (!ret) {
        if (!tableCellPrefab)
            tableCellPrefab = CreateTableCellPrefab();
        ret = UnityEngine::Object::Instantiate(tableCellPrefab);
    }
    ret->_reuseIdentifier = reuseIdentifier;
    auto cell = (GlobalNamespace::LevelListTableCell*) ret.ptr();

    auto content = models[idx];
    cell->_songNameText->text = content->Name();
    cell->_songAuthorText->text = content->Author();
    cell->_coverImage->sprite = content->Cover();

    auto image = (BSML::ClickableImage*) cell->_favoritesBadgeImage.ptr();
    image->gameObject->active = content->Deletable();
    image->onClick.clear();
    image->onClick += [this, idx]() {
        modelDeleted(idx);
    };

    return ret;
}

void SelectionSettings::DidActivate(bool firstActivation, bool, bool) {
    LoadManifests();
    if (firstActivation) {
        SetupFields();
        BSML_FILE(selection);
    } else
        RefreshModelList(true);
}

void SelectionSettings::SetupFields() {
    modelTypeData = ListW<HMUI::IconSegmentedControl::DataItem*>::New(3);
    modelTypeData->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(PNG_SPRITE(icons::saber), "Sabers", true));
    modelTypeData->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(PNG_SPRITE(icons::note), "Notes", true));
    modelTypeData->Add(HMUI::IconSegmentedControl::DataItem::New_ctor(PNG_SPRITE(icons::wall), "Walls", true));
    reuseIdentifier = "CustomModelsTableView";
    search = "";
}

void SelectionSettings::PostParse() {
    NULL_CHECK(searchHorizontal, sabersHorizontal, modelList, modelTypeSelector);

    SetupIcons(modelTypeSelector, SettingsCoordinator::GetInstance()->modelType);
    searchInput =
        BSML::Lite::CreateStringSetting(searchHorizontal, "Search", "", {0, 0}, {-15, -40, 0}, [this](StringW value) { searchInputTyped(value); });

    UnityEngine::Object::DestroyImmediate(modelList->GetComponent<BSML::CustomListTableData*>());
    tableView = modelList->GetComponentInChildren<HMUI::TableView*>();
    tableView->SetDataSource((HMUI::TableView::IDataSource*) this, false);
    RefreshModelList(true);
}

void SelectionSettings::RefreshModelList(bool full) {
    auto pair = GetSelectionOptions(search, full);
    models = pair.first;
    selectedModel = pair.second;

    if (!tableView)
        return;

    tableView->ReloadData();  // make sure to do this before anything else that would call RefreshCells after changing the data!
    if (selectedModel >= 0)
        tableView->SelectCellWithIdx(selectedModel, false);
    else
        tableView->ClearSelection();
    tableView->ScrollToCellWithIdx(std::max(selectedModel, 0), HMUI::TableView::ScrollPositionType::Center, false);
}

void SelectionSettings::OnDestroy() {
    instance = nullptr;
}

SelectionSettings* SelectionSettings::GetInstance() {
    return LazyCreate(instance, "CustomModelsSelectionSettings");
}

void SelectionSettings::modelTypeSelected(HMUI::SegmentedControl*, int idx) {
    if (sabersHorizontal)
        sabersHorizontal->gameObject->active = idx == 0;
    // force the vertical layout to change the sizes now so that the scroll view updates correctly
    if (layout)
        UnityEngine::UI::LayoutRebuilder::ForceRebuildLayoutImmediate(layout);
    SettingsCoordinator::GetInstance()->modelType = idx;
    RefreshModelList(false);
    ModSettings::GetInstance()->Refresh();
    PreviewSettings::GetInstance()->Refresh(true);
}

void SelectionSettings::searchInputTyped(StringW value) {
    search = value;
    RefreshModelList(false);
}

void SelectionSettings::saberOrTrailSelected(HMUI::SegmentedControl*, int idx) {
    SettingsCoordinator::GetInstance()->trail = idx == 1;
    RefreshModelList(false);
    ModSettings::GetInstance()->Refresh();
}

void SelectionSettings::menuPointerSelected(HMUI::SegmentedControl*, int idx) {
    SettingsCoordinator::GetInstance()->menuPointer = idx == 1;
    RefreshModelList(false);
    ModSettings::GetInstance()->Refresh();
    PreviewSettings::GetInstance()->Refresh(true);
}

void SelectionSettings::modelSelected(HMUI::TableView*, int idx) {
    selectedModel = idx;
    models[idx]->Select([]() {
        PreviewSettings::GetInstance()->Refresh(true);
        if (SettingsCoordinator::GetInstance()->modelType == 0 && SettingsCoordinator::GetInstance()->menuPointer)
            DestroyMenuPointers();
    });
}

void SelectionSettings::modelDeleted(int idx) {
    models[idx]->Delete();
    RefreshModelList(true);
    PreviewSettings::GetInstance()->Refresh(true);
    if (SettingsCoordinator::GetInstance()->modelType == 0 && SettingsCoordinator::GetInstance()->menuPointer)
        DestroyMenuPointers();
}

void ModSettings::DidActivate(bool firstActivation, bool, bool) {
    if (firstActivation)
        BSML_FILE(settings);
}

void ModSettings::PostParse() {
    NULL_CHECK(renameProfileButton, duplicateProfileButton, deleteProfileButton, profileNameModal);

    MetaCore::UI::ConvertToIconButton(renameProfileButton, PNG_SPRITE(icons::edit));
    MetaCore::UI::ConvertToIconButton(duplicateProfileButton, PNG_SPRITE(icons::copy));
    MetaCore::UI::ConvertToIconButton(deleteProfileButton, PNG_SPRITE(icons::delete));

    profileNameInput = BSML::Lite::CreateStringSetting(profileNameModal, "Name", "", {0, -6}, {0, 0, 0});
    MetaCore::UI::AddStringSettingOnClose(profileNameInput, nullptr, [this]() { nameModalClosed(profileNameInput->text); });

    UnityEngine::Color properDisabledColor = {0, 0, 0, 0.05};
    for (auto slider : {saberLengthSlider, saberWidthSlider, trailDurationSlider, trailWidthSlider, noteSizeSlider}) {
        if (!slider)
            continue;
        auto colors = slider->slider->colors;
        colors.disabledColor = properDisabledColor;
        slider->slider->colors = colors;
    }

    Refresh();
}

void ModSettings::Refresh() {
    NULL_CHECK(
        saberSettings,
        trailSettings,
        notesSettings,
        enableToggle,
        profileDropdown,
        deleteProfileButton,
        saberLengthSlider,
        saberLengthToggle,
        saberWidthSlider,
        saberWidthToggle,
        whiteTrailToggle,
        trailDurationSlider,
        trailDurationToggle,
        trailWidthSlider,
        trailWidthToggle,
        trailOffsetSlider,
        noteSizeSlider,
        noteSizeToggle,
        defaultBombsToggle,
        defaultDebrisToggle,
        wallsSettings,
        disableFrameToggle,
        disableCoreToggle
    );

    int modelType = SettingsCoordinator::GetInstance()->modelType;
    bool isTrail = SettingsCoordinator::GetInstance()->trail;
    saberSettings->active = modelType == 0 && !isTrail;
    trailSettings->active = modelType == 0 && isTrail;
    notesSettings->active = modelType == 1;
    wallsSettings->active = modelType == 2;

    MetaCore::UI::InstantSetToggle(enableToggle, getConfig().Enabled.GetValue());

    auto& profiles = getConfig().Profiles;
    std::vector<std::string> names;
    for (auto& [name, _] : profiles)
        names.emplace_back(name);
    MetaCore::UI::SetDropdownValues(profileDropdown, names, getConfig().Profile.GetValue());

    deleteProfileButton->interactable = profiles.size() > 1;

    auto& saber = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    saberLengthSlider->set_Value(saber.length);
    saberLengthSlider->set_interactable(saber.overrideLength);
    MetaCore::UI::InstantSetToggle(saberLengthToggle, saber.overrideLength);
    saberWidthSlider->set_Value(saber.width);
    saberWidthSlider->set_interactable(saber.overrideWidth);
    MetaCore::UI::InstantSetToggle(saberWidthToggle, saber.overrideWidth);

    auto& trail = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    MetaCore::UI::InstantSetToggle(whiteTrailToggle, !trail.whiteStep);
    trailDurationSlider->set_Value(trail.length);
    trailDurationSlider->set_interactable(trail.overrideLength);
    MetaCore::UI::InstantSetToggle(trailDurationToggle, trail.overrideLength);
    trailWidthSlider->set_Value(trail.width);
    trailWidthSlider->set_interactable(trail.overrideWidth);
    MetaCore::UI::InstantSetToggle(trailWidthToggle, trail.overrideWidth);
    trailOffsetSlider->set_Value(trail.widthOffset);

    auto& notes = getConfig().NotesSettings();
    noteSizeSlider->set_Value(notes.size);
    noteSizeSlider->set_interactable(notes.overrideSize);
    MetaCore::UI::InstantSetToggle(noteSizeToggle, notes.overrideSize);
    MetaCore::UI::InstantSetToggle(defaultBombsToggle, notes.defaultBombs);
    MetaCore::UI::InstantSetToggle(defaultDebrisToggle, notes.defaultDebris);

    auto& walls = getConfig().WallsSettings();
    MetaCore::UI::InstantSetToggle(disableFrameToggle, walls.disableFrame);
    MetaCore::UI::InstantSetToggle(disableCoreToggle, walls.disableCore);

    UnityEngine::UI::LayoutRebuilder::ForceRebuildLayoutImmediate(layout);
}

void ModSettings::OnDestroy() {
    instance = nullptr;
}

ModSettings* ModSettings::GetInstance() {
    return LazyCreate(instance, "CustomModelsModSettings");
}

void ModSettings::enableToggled(bool value) {
    getConfig().Enabled.SetValue(value);
}

void ModSettings::profileSelected(StringW value) {
    logger.debug("profile {} selected", value);
    getConfig().Profile.SetValue(value);
    Refresh();
    DestroyMenuPointers();
    SelectionSettings::GetInstance()->RefreshModelList(false);
    LoadSelections([]() { PreviewSettings::GetInstance()->Refresh(true); });
}

void ModSettings::renameProfilePressed() {
    NULL_CHECK(profileNameInput, profileNameModal);
    nameModalAction = 0;
    profileNameInput->text = getConfig().Profile.GetValue();
    profileNameModal->Show(true, false, nullptr);
}

void ModSettings::duplicateProfilePressed() {
    NULL_CHECK(profileNameInput, profileNameModal);
    nameModalAction = 1;
    profileNameInput->text = getConfig().Profile.GetValue();
    profileNameModal->Show(true, false, nullptr);
}

void ModSettings::deleteProfilePressed() {
    auto& profiles = getConfig().Profiles;
    auto profile = getConfig().Profile.GetValue();
    if (profiles.size() < 2 || !profiles.contains(profile))
        return;
    profiles.erase(profile);
    getConfig().Profile.SetValue(profiles.begin()->first);
    Refresh();
    SelectionSettings::GetInstance()->RefreshModelList(false);
    PreviewSettings::GetInstance()->Refresh(true);
}

void ModSettings::saberLengthChanged(float value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    settings.length = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::saberLengthToggled(bool value) {
    if (saberLengthSlider)
        saberLengthSlider->set_interactable(value);
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    settings.overrideLength = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::saberWidthChanged(float value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    settings.width = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::saberWidthToggled(bool value) {
    if (saberWidthSlider)
        saberWidthSlider->set_interactable(value);
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    settings.overrideWidth = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::whiteTrailToggled(bool value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.whiteStep = !value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::trailDurationChanged(float value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.length = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::trailDurationToggled(bool value) {
    if (trailDurationSlider)
        trailDurationSlider->set_interactable(value);
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.overrideLength = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::trailWidthChanged(float value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.width = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::trailWidthToggled(bool value) {
    if (trailWidthSlider)
        trailWidthSlider->set_interactable(value);
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.overrideWidth = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::trailOffsetChanged(float value) {
    auto& settings = SettingsCoordinator::GetInstance()->menuPointer ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    settings.widthOffset = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::noteSizeChanged(float value) {
    getConfig().NotesSettings().size = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::noteSizeToggled(bool value) {
    if (noteSizeSlider)
        noteSizeSlider->set_interactable(value);
    getConfig().NotesSettings().overrideSize = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::defaultBombsToggled(bool value) {
    getConfig().NotesSettings().defaultBombs = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::defaultDebrisToggled(bool value) {
    getConfig().NotesSettings().defaultDebris = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::disableFrameToggled(bool value) {
    getConfig().WallsSettings().disableFrame = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::disableCoreToggled(bool value) {
    getConfig().WallsSettings().disableCore = value;
    getConfig().Save();
    PreviewSettings::GetInstance()->Refresh(false);
}

void ModSettings::nameModalClosed(StringW value) {
    if (profileNameModal)
        profileNameModal->Hide(true, nullptr);
    std::string name = value;
    auto& profiles = getConfig().Profiles;
    auto profile = getConfig().Profile.GetValue();
    if (name.empty() || profiles.contains(name) || !profiles.contains(profile))
        return;
    profiles[name] = profiles[profile];
    if (nameModalAction == 0)
        profiles.erase(profile);
    getConfig().Profile.SetValue(name);
    Refresh();
    SelectionSettings::GetInstance()->RefreshModelList(false);
    PreviewSettings::GetInstance()->Refresh(true);
}

void PreviewSettings::DidActivate(bool firstActivation, bool, bool) {
    if (firstActivation)
        BSML_FILE(preview);
    else
        Refresh(true);
}

void PreviewSettings::DidDeactivate(bool, bool) {
    if (preview)
        UnityEngine::Object::Destroy(preview->gameObject);
    preview = nullptr;
}

void PreviewSettings::PostParse() {
    Refresh(true);
}

void PreviewSettings::Refresh(bool full) {
    logger.debug("refresh preview, full: {}", full);

    int modelType = SettingsCoordinator::GetInstance()->modelType;
    bool menu = SettingsCoordinator::GetInstance()->menuPointer;

    if (full) {
        if (preview)
            UnityEngine::Object::Destroy(preview->gameObject);
        preview = nullptr;
    }

    // I should probably replace these positions with an actual calculation using their curved canvas stuff but I don't want to
    switch (modelType) {
        case 0:
            if (!preview)
                preview = PreviewSabers({3.3, 0.8, 2.8}, UnityEngine::Quaternion::Euler({-90, 52, 0}), menu);
            UpdateSabersPreview(preview, menu);
            break;
        case 1:
            if (!preview)
                preview = PreviewNotes({2.7, 0.9, 3.6}, UnityEngine::Quaternion::Euler({0, 52, 0}));
            UpdateNotesPreview(preview);
            break;
        default:
            if (!preview)
                preview = PreviewWalls({3.3, 1.2, 2.8}, UnityEngine::Quaternion::Euler({0, 52, 0}));
            UpdateWallsPreview(preview);
            break;
    }
}

void PreviewSettings::OnDestroy() {
    if (preview)
        UnityEngine::Object::Destroy(preview->gameObject);
    instance = nullptr;
}

PreviewSettings* PreviewSettings::GetInstance() {
    return LazyCreate(instance, "CustomModelsPreviewSettings");
}

#pragma once

#include "GlobalNamespace/LevelListTableCell.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/ModalView.hpp"
#include "HMUI/TextSegmentedControl.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/Transform.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/ToggleSetting.hpp"
#include "custom-types/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(CustomModels, SettingsCoordinator, HMUI::FlowCoordinator) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::FlowCoordinator::DidActivate, bool firstActivation, bool, bool);
    DECLARE_OVERRIDE_METHOD_MATCH(void, BackButtonWasPressed, &HMUI::FlowCoordinator::BackButtonWasPressed, HMUI::ViewController* topViewController);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_STATIC_METHOD(void, Present);
    DECLARE_STATIC_METHOD(SettingsCoordinator*, GetInstance);

    DECLARE_INSTANCE_FIELD_DEFAULT(int, modelType, 0);
    DECLARE_INSTANCE_FIELD_DEFAULT(bool, trail, false);
    DECLARE_INSTANCE_FIELD_DEFAULT(bool, menuPointer, false);

    DECLARE_STATIC_METHOD(int, ModelSelection);

   private:
    static SettingsCoordinator* instance;
};

DECLARE_CLASS_CODEGEN_INTERFACES(CustomModels, SelectionSettings, HMUI::ViewController, HMUI::TableView::IDataSource*) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_OVERRIDE_METHOD_MATCH(float, CellSize, &HMUI::TableView::IDataSource::CellSize);
    DECLARE_OVERRIDE_METHOD_MATCH(int, NumberOfCells, &HMUI::TableView::IDataSource::NumberOfCells);
    DECLARE_OVERRIDE_METHOD_MATCH(HMUI::TableCell*, CellForIdx, &HMUI::TableView::IDataSource::CellForIdx, HMUI::TableView*, int idx);

    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool, bool);
    DECLARE_INSTANCE_METHOD(void, SetupFields);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_METHOD(void, RefreshModelList, bool full);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_STATIC_METHOD(SelectionSettings*, GetInstance);

    DECLARE_INSTANCE_FIELD(ListW<HMUI::IconSegmentedControl::DataItem*>, modelTypeData);

    DECLARE_INSTANCE_FIELD(GlobalNamespace::LevelListTableCell*, tableCellPrefab);
    DECLARE_INSTANCE_FIELD(StringW, reuseIdentifier);
    DECLARE_INSTANCE_FIELD(HMUI::TableView*, tableView);

    DECLARE_INSTANCE_FIELD(UnityEngine::RectTransform*, layout);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, searchHorizontal);
    DECLARE_INSTANCE_FIELD(HMUI::IconSegmentedControl*, modelTypeSelector);
    DECLARE_INSTANCE_FIELD(HMUI::InputFieldView*, searchInput);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, sabersHorizontal);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, modelList);

    DECLARE_INSTANCE_METHOD(void, modelTypeSelected, HMUI::SegmentedControl*, int idx);
    DECLARE_INSTANCE_METHOD(void, searchInputTyped, StringW value);
    DECLARE_INSTANCE_METHOD(void, saberOrTrailSelected, HMUI::SegmentedControl*, int idx);
    DECLARE_INSTANCE_METHOD(void, menuPointerSelected, HMUI::SegmentedControl*, int idx);
    DECLARE_INSTANCE_METHOD(void, modelSelected, HMUI::TableView*, int idx);
    DECLARE_INSTANCE_METHOD(void, modelDeleted, int idx);

    DECLARE_INSTANCE_FIELD(StringW, search);
    DECLARE_INSTANCE_FIELD(int, selectedModel);
    std::vector<class ListItem*> models;

   private:
    static SelectionSettings* instance;
};

DECLARE_CLASS_CODEGEN(CustomModels, ModSettings, HMUI::ViewController) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool, bool);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_METHOD(void, Refresh);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_STATIC_METHOD(ModSettings*, GetInstance);

    DECLARE_INSTANCE_FIELD(UnityEngine::RectTransform*, layout);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, enableToggle);
    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, profileDropdown);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, renameProfileButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, duplicateProfileButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, deleteProfileButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, saberSettings);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, saberLengthSlider);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, saberLengthToggle);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, saberWidthSlider);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, saberWidthToggle);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, trailSettings);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, whiteTrailToggle);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, trailDurationSlider);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, trailDurationToggle);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, trailWidthSlider);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, trailWidthToggle);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, trailOffsetSlider);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, notesSettings);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, noteSizeSlider);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, noteSizeToggle);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, defaultBombsToggle);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, defaultDebrisToggle);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, wallsSettings);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, disableFrameToggle);
    DECLARE_INSTANCE_FIELD(BSML::ToggleSetting*, disableCoreToggle);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, profileNameModal);
    DECLARE_INSTANCE_FIELD(HMUI::InputFieldView*, profileNameInput);

    DECLARE_INSTANCE_METHOD(void, enableToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, profileSelected, StringW value);
    DECLARE_INSTANCE_METHOD(void, renameProfilePressed);
    DECLARE_INSTANCE_METHOD(void, duplicateProfilePressed);
    DECLARE_INSTANCE_METHOD(void, deleteProfilePressed);
    DECLARE_INSTANCE_METHOD(void, saberLengthChanged, float value);
    DECLARE_INSTANCE_METHOD(void, saberLengthToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, saberWidthChanged, float value);
    DECLARE_INSTANCE_METHOD(void, saberWidthToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, whiteTrailToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, trailDurationChanged, float value);
    DECLARE_INSTANCE_METHOD(void, trailDurationToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, trailWidthChanged, float value);
    DECLARE_INSTANCE_METHOD(void, trailWidthToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, trailOffsetChanged, float value);
    DECLARE_INSTANCE_METHOD(void, noteSizeChanged, float value);
    DECLARE_INSTANCE_METHOD(void, noteSizeToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, defaultBombsToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, defaultDebrisToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, disableFrameToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, disableCoreToggled, bool value);
    DECLARE_INSTANCE_METHOD(void, nameModalClosed, StringW value);

    DECLARE_INSTANCE_FIELD(int, nameModalAction);

   private:
    static ModSettings* instance;
};

DECLARE_CLASS_CODEGEN(CustomModels, PreviewSettings, HMUI::ViewController) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation, bool, bool);
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidDeactivate, &HMUI::ViewController::DidDeactivate, bool, bool);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_METHOD(void, Refresh, bool full);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_STATIC_METHOD(PreviewSettings*, GetInstance);

    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, parent);
    DECLARE_INSTANCE_FIELD_DEFAULT(UnityEngine::Transform*, preview, nullptr);

   private:
    static PreviewSettings* instance;
};

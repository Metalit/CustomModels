#include "saber.hpp"

#include "GlobalNamespace/LayerMasks.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "UnityEngine/UI/Text.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "defaults.hpp"
#include "main.hpp"
#include "material.hpp"
#include "metacore/shared/operators.hpp"
#include "selection.hpp"
#include "trail.hpp"
#include "utils.hpp"

int const CustomModels::SaberLayer = GlobalNamespace::LayerMasks::GetLayer("Saber");

static inline void AddTrail(
    std::vector<CustomModels::Trail>& vec,
    CustomModels::TrailInfo trail,
    UnityEngine::Transform* saber,
    UnityEngine::Transform* parent,
    UnityEngine::Transform* top,
    UnityEngine::Transform* bottom
) {
    auto material = parent->GetComponent<UnityEngine::MeshRenderer*>()->material;
    auto topPos = top->position - saber->transform->position;
    auto bottomPos = bottom->position - saber->transform->position;
    vec.push_back({trail, material, topPos, bottomPos});
}

static void
AddLegacyTrails(UnityEngine::Transform* saber, std::vector<CustomModels::LegacyTrail> const& trails, int& id, std::vector<CustomModels::Trail>& out) {
    if (!saber)
        return;
    for (auto& trail : trails) {
        auto transform = saber->Find(trail.name);
        if (!transform) {
            logger.error("failed to find legacy trail {}", trail.name);
            continue;
        }
        auto top = transform->Find("TrailEnd");
        auto bottom = transform->Find("TrailStart");
        if (!top || !bottom) {
            logger.error("failed to find parts of legacy trail {}", trail.name);
            continue;
        }

        CustomModels::TrailInfo info;
        info.trailId = id++;
        info.colorType = trail.colorType;
        info.trailColor = trail.trailColor;
        info.multiplierColor = trail.multiplierColor;
        info.length = trail.length;
        info.whiteStep = trail.whiteStep;

        AddTrail(out, info, saber, transform, top, bottom);
    }
}

static void FindLegacyTrails(UnityEngine::AssetBundle* bundle, UnityEngine::GameObject* prefab, CustomModels::Saber& saber) {
    logger.debug("finding legacy trails");
    auto text = bundle->LoadAsset<UnityEngine::TextAsset*>("config");
    if (!text) {
        logger.warn("legacy config not found");
        return;
    }
    std::string json = text->text;
    CustomModels::LegacySaberConfig config;
    try {
        ReadFromString(json, config);
    } catch (std::exception const& e) {
        logger.error("failed to parse legacy config: {}", e.what());
        logger.debug("config was {}", json);
        return;
    }

    int id = 0;
    AddLegacyTrails(prefab->transform->Find("LeftSaber"), config.leftTrails, id, saber.leftTrails);
    AddLegacyTrails(prefab->transform->Find("RightSaber"), config.rightTrails, id, saber.rightTrails);
}

static void AddTrails(UnityEngine::Transform* saber, std::set<int>& ids, std::vector<CustomModels::Trail>& out) {
    std::vector<std::pair<UnityEngine::Transform*, CustomModels::TrailInfo>> trails;
    std::map<int, std::pair<UnityEngine::Transform*, UnityEngine::Transform*>> transforms;

    for (auto text : saber->GetComponentsInChildren<UnityEngine::UI::Text*>(true)) {
        std::string json = text->text;
        try {
            if (json.find("\"trailColor\"") != std::string::npos)
                trails.emplace_back(text->transform.ptr(), ReadFromString<CustomModels::TrailInfo>(json));
            else if (json.find("\"isTop\"") != std::string::npos) {
                auto object = ReadFromString<CustomModels::TrailObject>(json);
                if (object.isTop)
                    transforms[object.trailId].first = text->transform;
                else
                    transforms[object.trailId].second = text->transform;
            } else
                logger.warn("text did not match any trail json: {}", json);
        } catch (std::exception const& e) {
            logger.warn("failed to parse trail text: {}", e.what());
            logger.debug("text was {}", json);
        }
    }

    for (auto& [transform, trail] : trails) {
        auto& [top, bottom] = transforms[trail.trailId];
        if (!top || !bottom)
            logger.error("transforms for trail {} not found", trail.trailId);
        else if (ids.contains(trail.trailId))
            logger.warn("trail {} found multiple times", trail.trailId);
        else
            AddTrail(out, trail, saber, transform, top, bottom);
        ids.emplace(trail.trailId);
    }
}

static void FindTrails(UnityEngine::GameObject* prefab, CustomModels::Saber& saber) {
    logger.debug("finding custom trails");
    std::set<int> ids;
    AddTrails(prefab->transform->Find("LeftSaber"), ids, saber.leftTrails);
    AddTrails(prefab->transform->Find("RightSaber"), ids, saber.rightTrails);
}

static CustomModels::Trail GetDefaultTrail(int colorType) {
    CustomModels::TrailInfo info;
    info.trailId = colorType;
    info.colorType = colorType;
    info.trailColor = UnityEngine::Color::get_white();
    info.multiplierColor = UnityEngine::Color::get_white();
    info.length = 18;
    info.whiteStep = 0.03;

    return {info, CustomModels::GetDefaultTrailMaterial(), UnityEngine::Vector3::get_forward(), UnityEngine::Vector3::get_zero()};
}

void CustomModels::Saber::SetDefault() {
    asset.Unload();
    info.hasTrail = true;
    info.keepFakeGlow = true;
    info.isLegacy = false;
    leftTrails = {GetDefaultTrail(0)};
    rightTrails = {GetDefaultTrail(1)};
}

bool CustomModels::Saber::ParseInfo(std::string const& file) {
    try {
        info = files[file].config.Parse<SaberInfo>();
    } catch (std::exception const& e) {
        logger.error("failed to parse saber config for {}: {}", file, e.what());
        return false;
    }
    return true;
}

std::string CustomModels::Saber::ObjectName() {
    return info.isLegacy ? "_CustomSaber" : "_Whacker";
}

void CustomModels::Saber::PostLoad() {
    leftTrails.clear();
    rightTrails.clear();
    if (info.hasTrail)
        info.isLegacy ? FindLegacyTrails(asset.bundle, asset.asset, *this) : FindTrails(asset.asset, *this);
}

static void AddLoadingGuard(bool menu, bool trail) {
    CustomModels::Selection selection;
    if (trail)
        selection = menu ? CustomModels::Selection::MenuTrail : CustomModels::Selection::Trail;
    else
        selection = menu ? CustomModels::Selection::MenuSaber : CustomModels::Selection::Saber;
    getConfig().LoadingGuard.SetValue(CustomModels::assets[selection]->asset.currentFile);
}

static void RemoveLoadingGuard() {
    getConfig().LoadingGuard.SetValue("");
}

static CustomModels::CustomSaberTrail*
CreateTrail(UnityEngine::GameObject* saber, CustomModels::Trail const& trail, CustomModels::TrailSettings& settings) {
    auto parent = UnityEngine::GameObject::New_ctor(fmt::format("Trail{}", trail.info.trailId));
    auto component = parent->AddComponent<CustomModels::CustomSaberTrail*>();
    parent->transform->SetParent(saber->transform, false);

    parent->transform->localPosition = {0, 0, settings.widthOffset};

    auto top = UnityEngine::GameObject::New_ctor("CustomSaberTrailTop")->transform;
    top->SetParent(parent->transform, false);
    top->localPosition = trail.topPos;

    auto bottom = UnityEngine::GameObject::New_ctor("CustomSaberTrailBottom")->transform;
    bottom->SetParent(parent->transform, false);
    bottom->localPosition = trail.topPos - (trail.topPos - trail.bottomPos) * settings.Width();

    component->top = top;
    component->bottom = bottom;

    return component;
}

static void UpdateTrailColor(CustomModels::CustomSaberTrail* component, bool menu, CustomModels::Trail const& trail) {
    if (!component)
        return;
    auto leftColor = menu ? CustomModels::MenuLeftColor() : CustomModels::ColorScheme()->_saberAColor;
    auto rightColor = menu ? CustomModels::MenuRightColor() : CustomModels::ColorScheme()->_saberBColor;
    auto color = CustomModels::TrailColor(trail.info, leftColor, rightColor);
    auto otherColor = CustomModels::TrailColor(trail.info, rightColor, leftColor);
    component->SetColor(color, otherColor);
}

static void InitTrail(UnityEngine::GameObject* saber, bool menu, CustomModels::Trail const& trail) {
    auto& settings = menu ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
    auto component = CreateTrail(saber, trail, settings);

    CustomModels::SetLayerRecursively(component->transform, CustomModels::SaberLayer);

    component->_trailDuration = settings.Length() * trail.info.length / (float) 45;
    component->_whiteSectionMaxDuration = settings.whiteStep ? trail.info.whiteStep : 0;
    UpdateTrailColor(component, menu, trail);

    component->Init(trail.material);
}

static void ScaleSaber(UnityEngine::Transform* instance, bool menu) {
    auto& settings = menu ? getConfig().MenuSaberSettings() : getConfig().SaberSettings();
    instance->localScale = {settings.Width(), settings.Width(), settings.Length()};
}

static UnityEngine::GameObject* CreateSaber(bool menu, GlobalNamespace::SaberType type) {
    auto selection = menu ? CustomModels::Selection::MenuSaber : CustomModels::Selection::Saber;
    CustomModels::Saber* saber = dynamic_cast<CustomModels::Saber*>(CustomModels::assets[selection]);

    auto name = type == GlobalNamespace::SaberType::SaberA ? "LeftSaber" : "RightSaber";
    auto instance = saber->asset.InstantiateChild(name);
    if (!instance) {
        if (menu && !getConfig().MenuSaber())
            instance = UnityEngine::GameObject::New_ctor();
        else
            instance = CustomModels::GetDefaultSaber();
    }
    instance->name = name;
    return instance;
}

static CustomModels::Saber* GetTrailsInfo(bool menu) {
    int trailMode = menu ? getConfig().MenuTrailMode() : getConfig().TrailMode();
    CustomModels::Selection selection;
    switch (trailMode) {
        case 0:
            if (menu && !getConfig().MenuSaber())
                return nullptr;
            selection = menu ? CustomModels::Selection::MenuSaber : CustomModels::Selection::Saber;
            break;
        case 1:
            return nullptr;
        default:
            selection = menu ? CustomModels::Selection::MenuTrail : CustomModels::Selection::Trail;
            break;
    }
    return dynamic_cast<CustomModels::Saber*>(CustomModels::assets[selection]);
}

void CustomModels::InitSaber(UnityEngine::Transform* parent, bool menu, GlobalNamespace::SaberType type) {
    logger.info("custom saber init (menu: {})", menu);
    logger.debug("saber type: {}", (int) type);

    auto instance = CreateSaber(menu, type);

    auto transform = instance->transform;
    transform->SetParent(parent, false);
    SetLayerRecursively(transform, CustomModels::SaberLayer);
    ScaleSaber(transform, menu);

    bool left = type == GlobalNamespace::SaberType::SaberA;
    auto colors = instance->AddComponent<ColorVisuals*>();
    if (menu)
        colors->SetMenuColor(left);
    else
        colors->SetSidedColor(left);

    if (auto info = GetTrailsInfo(menu)) {
        logger.info("custom trails init");
        for (auto& trail : left ? info->leftTrails : info->rightTrails)
            InitTrail(instance, menu, trail);
    }
}

void CustomModels::UpdateSaberColor(UnityEngine::Transform* parent, bool menu, GlobalNamespace::SaberType type) {
    auto trails = GetTrailsInfo(menu);
    bool left = type == GlobalNamespace::SaberType::SaberA;

    auto saber = parent->Find(left ? "LeftSaber" : "RightSaber");
    if (!saber)
        return;

    auto colors = saber->GetComponent<ColorVisuals*>();
    if (menu)
        colors->SetMenuColor(left);
    else
        colors->SetSidedColor(left);

    if (auto info = GetTrailsInfo(menu)) {
        for (auto& trail : left ? info->leftTrails : info->rightTrails) {
            if (auto child = saber->Find(fmt::format("Trail{}", trail.info.trailId)))
                UpdateTrailColor(child->GetComponent<CustomSaberTrail*>(), menu, trail);
        }
    }
}

static void PreviewTrails(UnityEngine::GameObject* saber, std::vector<CustomModels::Trail>& trails, CustomModels::TrailSettings& settings) {
    for (auto& trail : trails) {
        auto component = CreateTrail(saber, trail, settings);
        auto color = CustomModels::TrailColor(trail.info, CustomModels::MenuLeftColor(), CustomModels::MenuRightColor());
        auto otherColor = CustomModels::TrailColor(trail.info, CustomModels::MenuRightColor(), CustomModels::MenuLeftColor());
        component->SetStatic(true);
        component->SetColor(color, otherColor);
        component->Init(trail.material);
    }
}

UnityEngine::Transform* CustomModels::PreviewSabers(UnityEngine::Vector3 position, UnityEngine::Quaternion rotation, bool menu) {
    logger.debug("creating saber preview");

    AddLoadingGuard(menu, false);

    auto preview = UnityEngine::GameObject::New_ctor("CustomModelsPreview")->transform;
    SetLayerRecursively(preview, CustomModels::SaberLayer);

    auto leftSaber = CreateSaber(menu, GlobalNamespace::SaberType::SaberA);
    auto rightSaber = CreateSaber(menu, GlobalNamespace::SaberType::SaberB);

    leftSaber->AddComponent<ColorVisuals*>()->SetMenuColor(true);
    rightSaber->AddComponent<ColorVisuals*>()->SetMenuColor(false);

    leftSaber->transform->SetParent(preview, false);
    leftSaber->transform->localPosition = {-0.25, 0, 0};
    leftSaber->transform->localEulerAngles = {0, 0, 180};
    rightSaber->transform->SetParent(preview, false);
    rightSaber->transform->localPosition = {0.25, 0, 0};

    if (auto info = GetTrailsInfo(menu)) {
        AddLoadingGuard(menu, true);

        logger.info("preview trails init");
        auto& settings = menu ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();
        PreviewTrails(leftSaber, info->leftTrails, settings);
        PreviewTrails(rightSaber, info->rightTrails, settings);
    }

    RemoveLoadingGuard();

    preview->SetPositionAndRotation(position, rotation);
    return preview;
}

static void UpdateTrailPreview(UnityEngine::Transform* parent, CustomModels::Trail& trail, bool menu) {
    auto& settings = menu ? getConfig().MenuTrailSettings() : getConfig().TrailSettings();

    parent->localPosition = {0, 0, settings.widthOffset};

    auto bottom = parent->Find("CustomSaberTrailBottom");
    bottom->localPosition = trail.topPos - (trail.topPos - trail.bottomPos) * settings.Width();

    auto component = parent->GetComponent<CustomModels::CustomSaberTrail*>();

    component->_trailDuration = settings.Length() * trail.info.length / (float) 45;
    component->_whiteSectionMaxDuration = settings.whiteStep ? trail.info.whiteStep : 0;
    component->RefreshTrail();
}

void CustomModels::UpdateSabersPreview(UnityEngine::Transform* preview, bool menu) {
    auto trails = GetTrailsInfo(menu);
    bool left = true;

    for (auto name : {"LeftSaber", "RightSaber"}) {
        AddLoadingGuard(menu, true);

        auto saber = preview->Find(name);
        if (!saber)
            continue;
        ScaleSaber(saber, menu);

        if (trails) {
            AddLoadingGuard(menu, false);
            for (auto& trail : left ? trails->leftTrails : trails->rightTrails) {
                if (auto child = saber->Find(fmt::format("Trail{}", trail.info.trailId)))
                    UpdateTrailPreview(child, trail, menu);
            }
        }

        left = false;
    }

    RemoveLoadingGuard();
}

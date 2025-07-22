#include "wall.hpp"

#include "GlobalNamespace/ObstacleMaterialSetter.hpp"
#include "GlobalNamespace/SettingsManager.hpp"
#include "GlobalNamespace/StretchableObstacle.hpp"
#include "UnityEngine/MeshFilter.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Renderer.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "defaults.hpp"
#include "main.hpp"
#include "material.hpp"
#include "metacore/shared/game.hpp"
#include "metacore/shared/unity.hpp"
#include "utils.hpp"

void CustomModels::Wall::SetDefault() {
    asset.Unload();
    info.replaceCoreMaterial = false;
    info.replaceFrameMaterial = false;
    info.replaceCoreMesh = false;
    info.replaceFrameMesh = false;
    info.disableCore = false;
    info.disableFrame = false;
    info.disableFakeGlow = false;
    info.isMirrorable = true;
    info.isLegacy = false;
}

bool CustomModels::Wall::ParseInfo(std::string const& file) {
    try {
        info = files[file].config.Parse<WallInfo>();
    } catch (std::exception const& e) {
        logger.error("failed to parse wall config for {}: {}", file, e.what());
        return false;
    }
    return true;
}

std::string CustomModels::Wall::ObjectName() {
    return info.isLegacy ? "_CustomWall" : "_Box";
}

void CustomModels::Wall::PostLoad() {}  // nothing to change

static void SetMesh(UnityEngine::Transform* object, UnityEngine::GameObject* copy) {
    object->GetComponent<UnityEngine::MeshFilter*>()->mesh = copy ? copy->GetComponent<UnityEngine::MeshFilter*>()->sharedMesh : nullptr;
}

static void SetMaterial(UnityEngine::Transform* object, UnityEngine::GameObject* copy, bool mirror = false) {
    auto copied = copy->GetComponent<UnityEngine::MeshRenderer*>()->GetMaterialArray();
    if (mirror) {
        for (auto material : copied)
            CustomModels::SetMirrorableProperties(material, true);
    }
    object->GetComponent<UnityEngine::MeshRenderer*>()->SetMaterialArray(copied.convert());  // UnityW inconsistency
}

#include "GlobalNamespace/StretchableObstacle.hpp"

void CustomModels::InitWall(UnityEngine::GameObject* prefab, bool mirror) {
    logger.info("custom wall init, mirror: {}", mirror);

    Wall* wall = dynamic_cast<Wall*>(assets[Selection::Wall]);

    if (mirror && !wall->info.isMirrorable)
        return;

    auto transform = prefab->transform;

    auto frame = transform->Find(mirror ? "ObstacleFrame" : "HideWrapper/ObstacleFrame");
    auto customFrame = wall->asset.GetChild("Frame");

    if (!customFrame && (wall->info.replaceFrameMesh || wall->info.replaceFrameMaterial)) {
        logger.error("custom wall frame not found");
        return;
    }

    if (!wall->info.disableFrame && !getConfig().WallsSettings().disableFrame) {
        if (!mirror && wall->info.disableFakeGlow)
            SetMesh(transform->Find("HideWrapper/ObstacleFakeGlow"), nullptr);
        if (wall->info.replaceFrameMesh)
            SetMesh(frame, customFrame);
        if (wall->info.replaceFrameMaterial)
            SetMaterial(frame, customFrame, mirror);
    } else {
        SetMesh(frame, nullptr);
        if (!mirror && getConfig().WallsSettings().disableFrame)
            SetMesh(transform->Find("HideWrapper/ObstacleFakeGlow"), nullptr);
    }

    // seems like mirrored walls don't have cores
    if (!mirror) {
        auto core = transform->Find("ObstacleCore");
        core->Find("DepthWrite")->GetComponent<UnityEngine::Renderer*>()->enabled = false;
        auto customCore = wall->asset.GetChild("Core");

        if (!customCore && (wall->info.replaceCoreMesh || wall->info.replaceCoreMaterial)) {
            logger.error("custom wall core not found");
            return;
        }

        if (!wall->info.disableCore && !getConfig().WallsSettings().disableCore) {
            if (wall->info.replaceCoreMesh)
                SetMesh(core, customCore);
            if (wall->info.replaceCoreMaterial) {
                auto switcher = prefab->GetComponent<GlobalNamespace::ObstacleMaterialSetter*>();
                switcher->_lwCoreMaterial = nullptr;
                switcher->_texturedCoreMaterial = nullptr;
                switcher->_hwCoreMaterial = nullptr;
                SetMaterial(core, customCore);
            }
        } else
            SetMesh(core, nullptr);
    }

    prefab->AddComponent<ColorVisuals*>()->SetColor(ColorScheme()->_obstaclesColor, UnityEngine::Color::get_black());
}

UnityEngine::Transform* CustomModels::PreviewWalls(UnityEngine::Vector3 position, UnityEngine::Quaternion rotation) {
    logger.debug("creating wall preview");

    auto preview = UnityEngine::GameObject::New_ctor("CustomModelsPreview")->transform;
    auto scale = UnityEngine::Vector3(1.5, 1, 0.5);

    Wall* wall = dynamic_cast<Wall*>(assets[Selection::Wall]);
    if (wall->asset.asset) {
        auto instance = wall->asset.Instantiate()->transform;

        MetaCore::Engine::DisableAllBut(instance, {"Core", "Frame"});
        instance->SetParent(preview, false);

        instance->localScale = scale;

        // 0.05 appears to be related to the frame
        auto size = UnityEngine::Vector4(scale.x / 2, scale.y / 2, scale.z / 2, 0.05);
        for (auto renderer : instance->GetComponentsInChildren<UnityEngine::Renderer*>()) {
            for (auto material : renderer->materials) {
                if (material->HasVector(Material::SizeParams))
                    material->SetVector(Material::SizeParams, size);
            }
        }

        instance->gameObject->AddComponent<ColorVisuals*>()->SetColor(MenuWallColor(), UnityEngine::Color::get_black());
    } else {
        auto instance = GetDefaultWall()->transform;
        instance->SetParent(preview, false);

        auto stretch = instance->GetComponent<GlobalNamespace::StretchableObstacle*>();
        stretch->SetAllProperties(scale.x, scale.y, scale.z, MenuWallColor(), 0);

        auto switcher = instance->GetComponent<GlobalNamespace::ObstacleMaterialSetter*>();
        auto quality = MetaCore::Game::GetMainSystemInit()->_settingsManager->settings.quality.obstacles;
        logger.debug("found quality {}", (int) quality);
        switcher->Init(quality, true);

        instance->localPosition = {0, -scale.y / 2, -scale.z / 2};
    }

    preview->SetPositionAndRotation(position, rotation);
    return preview;
}

void CustomModels::UpdateWallsPreview(UnityEngine::Transform* preview) {
    auto instance = preview->GetChild(0);
    auto& settings = getConfig().WallsSettings();

    auto frame = instance->Find("Frame");
    if (!frame)
        frame = instance->Find("HideWrapper/ObstacleFrame");
    if (frame)
        frame->gameObject->active = !settings.disableFrame;
    if (auto glow = instance->Find("HideWrapper/ObstacleFakeGlow"))
        glow->gameObject->active = !settings.disableFrame;

    auto core = instance->Find("Core");
    if (!core)
        core = instance->Find("ObstacleCore");
    if (core)
        core->gameObject->active = !settings.disableCore;
}

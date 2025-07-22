#pragma once

#include "GlobalNamespace/SaberTrail.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/Transform.hpp"
#include "custom-types/shared/macros.hpp"
#include "json.hpp"

DECLARE_CLASS_CODEGEN(CustomModels, CustomSaberTrail, GlobalNamespace::SaberTrail) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_INSTANCE_METHOD(void, Init, UnityEngine::Material* material);
    DECLARE_INSTANCE_METHOD(void, SetStatic, bool value);
    DECLARE_INSTANCE_METHOD(void, SetColor, UnityEngine::Color color, UnityEngine::Color otherColor);
    DECLARE_INSTANCE_METHOD(void, RefreshTrail);
    // called by a hook right before LateUpdate, to be compatible with mods (replay) that move the saber in Update
    DECLARE_INSTANCE_METHOD(bool, PreLateUpdate);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_INSTANCE_FIELD(int, trailId);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, top);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, bottom);
    DECLARE_INSTANCE_FIELD_DEFAULT(UnityEngine::Mesh*, staticMesh, nullptr);
    DECLARE_INSTANCE_FIELD_DEFAULT(bool, staticTrail, false);
    DECLARE_INSTANCE_FIELD_DEFAULT(bool, initialized, false);
};

namespace CustomModels {
    UnityEngine::Color TrailColor(TrailInfo const& info, UnityEngine::Color const& leftColor, UnityEngine::Color const& rightColor);
}

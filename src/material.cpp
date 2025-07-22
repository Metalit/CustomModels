#include "material.hpp"

#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Resources.hpp"
#include "main.hpp"
#include "metacore/shared/strings.hpp"

#define PROPERTY_ID(identifier) \
    int const CustomModels::Material::identifier = UnityEngine::Shader::PropertyToID("_" #identifier);

PROPERTY_ID(CustomColors);
PROPERTY_ID(Glow);
PROPERTY_ID(Bloom);
PROPERTY_ID(Color);
PROPERTY_ID(OtherColor);
PROPERTY_ID(Alpha);
PROPERTY_ID(StencilRefID);
PROPERTY_ID(StencilComp);
PROPERTY_ID(StencilOp);
PROPERTY_ID(BlendSrcFactor);
PROPERTY_ID(BlendDstFactor);
PROPERTY_ID(BlendSrcFactorA);
PROPERTY_ID(BlendDstFactorA);
PROPERTY_ID(SlicePos);
PROPERTY_ID(CutPlane);
PROPERTY_ID(TransformOffset);
PROPERTY_ID(SizeParams);

bool CustomModels::ShouldColor(UnityEngine::Material* material) {
    if (!material)
        return false;
    for (auto& property : {CustomModels::Material::CustomColors, CustomModels::Material::Glow, CustomModels::Material::Bloom}) {
        if (material->HasFloat(property))
            return material->GetFloat(property) > 0;
    }
    return false;
}

bool CustomModels::ShouldColorReplaced(UnityEngine::Material* material) {
    if (!material)
        return false;
    if (!material->name->Contains("_replace"))
        return false;
    return !material->name->Contains("_noCC");
}

static void SetFloat(UnityEngine::Material* material, int property, float value) {
    if (material->HasFloat(property))
        material->SetFloat(property, value);
}

void CustomModels::SetMirrorableProperties(UnityEngine::Material* material, bool mirror) {
    material->renderQueue = mirror ? 1951 : 2004;
    SetFloat(material, CustomModels::Material::Alpha, mirror ? 0.05 : 1.0);
    SetFloat(material, CustomModels::Material::StencilRefID, mirror ? 1 : 0);
    SetFloat(material, CustomModels::Material::StencilComp, mirror ? 3 : 8);
    SetFloat(material, CustomModels::Material::StencilOp, 0);
    SetFloat(material, CustomModels::Material::BlendDstFactor, mirror ? 10 : 0);
    SetFloat(material, CustomModels::Material::BlendDstFactorA, 0);
    SetFloat(material, CustomModels::Material::BlendSrcFactor, mirror ? 5 : 1);
    SetFloat(material, CustomModels::Material::BlendSrcFactorA, 0);
}

static ArrayW<UnityEngine::Material*> allMaterials;
// cache for reused replacements
static std::map<std::string, UnityEngine::Material*> replacements;

static UnityEngine::Material* FindReplacement(std::string const& name) {
    if (!allMaterials)
        allMaterials = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>();

    if (replacements.contains(name))
        return replacements[name];

    UnityEngine::Material* ret = allMaterials->FirstOrDefault([&name](UnityEngine::Material* material) {
        return material->name->Length > 0 && !material->name->Contains("_replace") &&
               name.find((std::string) material->name->ToLower()) != std::string::npos;
    });
    replacements[name] = ret;
    return ret;
}

static void ReplaceMaterials(UnityEngine::Renderer* renderer) {
    auto materials = renderer->materials;
    bool changed = false;

    for (auto& material : materials) {
        std::string name = material->name->ToLower();
        if (name.find("_replace") == std::string::npos || name.find("_done") != std::string::npos)
            continue;

        logger.debug("searching for replacement for {}", name);
        auto replacement = FindReplacement(name);
        name += "_done";
        if (name.find("_noCC") == std::string::npos && !CustomModels::ShouldColor(material))
            name += "_noCC";

        if (!replacement) {
            logger.debug("no replacement found");
            material->name = name;
            continue;
        }
        logger.debug("found replacement {}", replacement->name);
        auto color = material->color;
        material = UnityEngine::Object::Instantiate(replacement);
        material->name = name;
        material->color = color;
        changed = true;
    }

    if (changed)
        renderer->materials = materials.convert();  // UnityW inconsistency
}

void CustomModels::ReplaceMaterials(UnityEngine::GameObject* object) {
    // some custom notes want to use copies of game materials
    // if they want to use the game material "NoteMaterial", they can have a material named "NoteMaterial_replace"
    logger.info("replacing materials for object {}", object->name);

    auto renderers = object->GetComponentsInChildren<UnityEngine::Renderer*>(true);
    for (auto renderer : renderers)
        ReplaceMaterials(renderer);
}

void CustomModels::ClearMaterialsCache() {
    replacements.clear();
    allMaterials = nullptr;
}

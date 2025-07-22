#include "note.hpp"

#include "GlobalNamespace/ColorNoteVisuals.hpp"
#include "GlobalNamespace/ILazyCopyHashSet_1.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "UnityEngine/MaterialPropertyBlock.hpp"
#include "UnityEngine/MeshFilter.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "defaults.hpp"
#include "main.hpp"
#include "material.hpp"
#include "metacore/shared/operators.hpp"
#include "registration.hpp"
#include "utils.hpp"

DEFINE_TYPE(CustomModels, NoteVisuals);
DEFINE_TYPE(CustomModels, DebrisVisuals);

static std::vector<std::string> const NoteChildren = {"LeftArrow", "RightArrow", "LeftDot", "RightDot"};
static std::vector<std::string> const ChainChildren = {"LeftHead", "RightHead", "LeftLink", "RightLink"};

void CustomModels::NoteVisuals::ShowObject(int index) {
    if (index == current)
        return;
    for (int i = 0; i < objects.size(); i++)
        objects[i]->active = i == index;
    current = index;
}

void CustomModels::NoteVisuals::HandleNoteControllerDidInit(GlobalNamespace::NoteControllerBase* note) {
    int index = note->noteData->colorType == GlobalNamespace::ColorType::ColorA ? 0 : 1;
    if (isChain)
        index += note->noteData->gameplayType == GlobalNamespace::NoteData::GameplayType::BurstSliderHead ? 0 : 2;
    else
        index += note->noteData->cutDirection == GlobalNamespace::NoteCutDirection::Any ? 2 : 0;
    ShowObject(index);
}

void CustomModels::NoteVisuals::Awake() {
    if (auto note = GetComponentInParent<GlobalNamespace::NoteControllerBase*>())
        note->didInitEvent->Add((GlobalNamespace::INoteControllerDidInitEvent*) this);

    isChain = name->Contains("Chain");
    auto const& names = isChain ? ChainChildren : NoteChildren;

    objects = ArrayW<UnityEngine::GameObject*>(names.size());
    for (int i = 0; i < names.size(); i++) {
        if (auto child = transform->Find(names[i]))
            objects[i] = child->gameObject;
        else
            logger.critical("missing child {}", names[i]);  // will still crash
    }

    for (int i = 0; i < objects.size(); i++) {
        objects[i]->AddComponent<ColorVisuals*>()->SetSidedColor(i % 2 == 0);
        objects[i]->active = false;
    }
}

void CustomModels::NoteVisuals::OnDestroy() {
    if (auto note = GetComponentInParent<GlobalNamespace::NoteControllerBase*>())
        note->didInitEvent->Remove((GlobalNamespace::INoteControllerDidInitEvent*) this);
}

void CustomModels::DebrisVisuals::ShowDebris(GlobalNamespace::ColorType color, UnityEngine::Vector3 cutPoint, UnityEngine::Vector3 cutNormal) {
    for (int i = 0; i < objects.size(); i++)
        objects[i]->active = i == (int) color;

    auto debris = objects[(int) color];
    float magnitude = cutPoint.magnitude;
    if (magnitude > 0.04)
        cutPoint = cutPoint * 0.2 / std::sqrt(magnitude);

    UnityEngine::Vector4 slicePos = {cutPoint.x, cutPoint.y, cutPoint.z, 0};
    UnityEngine::Vector4 cutPlane = {cutNormal.x, cutNormal.y, cutNormal.z, 0};

    for (auto& [renderer, materials] : color == GlobalNamespace::ColorType::ColorA ? leftMaterials : rightMaterials) {
        auto pos = renderer->transform->localPosition;
        UnityEngine::Vector4 transformOffset = {pos.x, pos.y, pos.z, 0};

        for (auto& material : materials) {
            material->SetVector(Material::SlicePos, slicePos);
            material->SetVector(Material::CutPlane, cutPlane);
            material->SetVector(Material::TransformOffset, transformOffset);
        }
    }
}

static std::map<UnityEngine::Renderer*, std::vector<UnityEngine::Material*>> GetSlicableMaterials(UnityEngine::GameObject* debris) {
    std::map<UnityEngine::Renderer*, std::vector<UnityEngine::Material*>> ret;
    for (auto renderer : debris->GetComponentsInChildren<UnityEngine::Renderer*>(true)) {
        for (auto material : renderer->materials) {
            if (material->HasVector(CustomModels::Material::SlicePos) && material->HasVector(CustomModels::Material::CutPlane) &&
                material->HasVector(CustomModels::Material::TransformOffset))
                ret[renderer].emplace_back(material);
        }
    }
    return ret;
}

void CustomModels::DebrisVisuals::Awake() {
    objects = ArrayW<UnityEngine::GameObject*>(2);
    objects[0] = transform->Find("LeftDebris")->gameObject;
    objects[1] = transform->Find("RightDebris")->gameObject;

    leftMaterials = GetSlicableMaterials(objects[0]);
    rightMaterials = GetSlicableMaterials(objects[1]);

    objects[0]->AddComponent<ColorVisuals*>()->SetSidedColor(true);
    objects[1]->AddComponent<ColorVisuals*>()->SetSidedColor(false);
}

static void FixLegacyObjects(UnityEngine::Transform* prefab) {
    logger.debug("performing legacy fixes");

    auto notes = UnityEngine::GameObject::New_ctor("Notes")->transform;
    notes->SetParent(prefab, false);

    for (auto& name : {"LeftArrow", "RightArrow", "LeftDot", "RightDot"})
        prefab->Find(name)->SetParent(notes, false);

    auto leftDebris = prefab->Find("LeftDebris");
    auto rightDebris = prefab->Find("RightDebris");
    if (!leftDebris || !rightDebris)
        return;

    auto debris = UnityEngine::GameObject::New_ctor("Debris")->transform;
    debris->SetParent(prefab, false);
    leftDebris->SetParent(debris, false);
    rightDebris->SetParent(debris, false);
}

static void CreateChain(UnityEngine::Transform* parent, UnityEngine::Transform* base, std::string const& name, bool link, bool debris) {
    if (!base) {
        logger.error("base note for chain {} not found", name);
        return;
    }
    auto unscaled = UnityEngine::GameObject::New_ctor(name)->transform;
    unscaled->parent = parent;
    auto note = CustomModels::Instantiate(base, unscaled);
    note->name = name;
    note->localPosition = {0, (float) (link || debris ? 0 : 0.125), 0};
    note->localScale = {1, (float) (link ? 0.2 : 0.75), 1};
}

static void CreateChains(UnityEngine::Transform* prefab, CustomModels::NoteInfo& info) {
    logger.debug("creating chains");

    auto notes = prefab->Find("Notes");

    auto chains = prefab->Find("Chains");
    if (!chains) {
        chains = UnityEngine::GameObject::New_ctor("Chains")->transform;
        chains->SetParent(prefab, false);
    }

    // create chains from scaled/moved copies of non-chains
    for (int i = 0; i < NoteChildren.size(); i++) {
        auto base = NoteChildren[i];
        auto part = ChainChildren[i];
        bool link = i >= 2;
        if (!chains->Find(part))
            CreateChain(chains, notes->Find(base), part, link, false);
    }
    info.hasSlider = true;

    if (!info.hasDebris)
        return;
    auto debris = prefab->Find("Debris");
    if (!debris) {
        logger.error("expected debris was not found");
        return;
    }

    // create left and right debris copies for chain heads and links
    auto left = debris->Find("LeftDebris");
    auto right = debris->Find("RightDebris");
    if (!left || !right) {
        logger.error("debris children not found: {} {}", (bool) left, (bool) right);
        return;
    }

    for (auto& [name, link] : std::vector<std::tuple<std::string, bool>>{{"ChainHeadDebris", false}, {"ChainLinkDebris", true}}) {
        if (!prefab->Find(name)) {
            auto parent = UnityEngine::GameObject::New_ctor(name)->transform;
            parent->SetParent(prefab, false);
            CreateChain(parent, left, "LeftDebris", link, true);
            CreateChain(parent, right, "RightDebris", link, true);
        }
    }
    info.hasChainHeadDebris = true;
    info.hasChainLinkDebris = true;
}

void SetAllMirrorableProperties(UnityEngine::Transform* object, bool mirror) {
    auto renderers = object->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

    for (auto renderer : renderers) {
        for (auto material : renderer->materials)
            CustomModels::SetMirrorableProperties(material, mirror);
    }
}

static void DuplicateForMirror(UnityEngine::Transform* prefab, std::string const& baseName, std::string const& targetName) {
    if (prefab->Find(targetName))
        return;

    auto base = prefab->Find(baseName);
    if (!base) {
        logger.error("base object {} for mirror not found", baseName);
        return;
    }

    auto mirror = CustomModels::Instantiate(base, prefab);
    mirror->name = targetName;

    SetAllMirrorableProperties(base, false);
    SetAllMirrorableProperties(mirror, true);
}

static void CreateMirrors(UnityEngine::Transform* prefab) {
    logger.debug("creating mirrors");

    DuplicateForMirror(prefab, "Notes", "MirrorNotes");
    DuplicateForMirror(prefab, "Bomb", "MirrorBomb");
    DuplicateForMirror(prefab, "Chains", "MirrorChains");
}

static void ResetObjectPositions(UnityEngine::Transform* prefab) {
    for (int i = 0; i < prefab->childCount; i++) {
        auto parent = prefab->GetChild(i);
        for (int j = 0; j < parent->childCount; j++) {
            auto child = parent->GetChild(j);
            child->localScale = UnityEngine::Vector3::get_one();
            child->localPosition = UnityEngine::Vector3::get_zero();
            child->localRotation = UnityEngine::Quaternion::get_identity();
        }
    }
}

void CustomModels::Note::SetDefault() {
    asset.Unload();
    info.hasDebris = true;
    info.hasChainHeadDebris = true;
    info.hasChainLinkDebris = true;
    info.hasSlider = true;
    info.hasBomb = true;
    info.showArrows = true;
    info.isMirrorable = true;
    info.isLegacy = false;
}

bool CustomModels::Note::ParseInfo(std::string const& file) {
    try {
        info = files[file].config.Parse<NoteInfo>();
    } catch (std::exception const& e) {
        logger.error("failed to parse note config for {}: {}", file, e.what());
        return false;
    }
    return true;
}

std::string CustomModels::Note::ObjectName() {
    return info.isLegacy ? "_CustomBloq" : "_Cyoob";
}

void CustomModels::Note::PostLoad() {
    auto prefab = asset.asset->transform;
    if (info.isLegacy)
        FixLegacyObjects(prefab);
    if (!info.hasSlider)
        CreateChains(prefab, info);
    if (info.isMirrorable)
        CreateMirrors(prefab);
    ResetObjectPositions(prefab);
}

static void AddLoadingGuard() {
    getConfig().LoadingGuard.SetValue(CustomModels::assets[CustomModels::Selection::Note]->asset.currentFile);
}

static void RemoveLoadingGuard() {
    getConfig().LoadingGuard.SetValue("");
}

void CustomModels::InitNote(UnityEngine::GameObject* prefab, NoteType type, bool debris, bool mirror) {
    logger.info("custom note init {}, debris: {}, mirror: {}", (int) type, debris, mirror);

    std::string childName;
    std::string parentName;

    switch (type) {
        case NoteType::Note:
            childName = debris ? "Debris" : mirror ? "MirrorNotes" : "Notes";
            parentName = debris ? "NoteDebrisMesh" : "NoteCube";
            break;
        case NoteType::Bomb:
            childName = mirror ? "MirrorBomb" : "Bomb";
            parentName = "Mesh";
            break;
        case NoteType::ChainHead:
            childName = debris ? "ChainHeadDebris" : mirror ? "MirrorChains" : "Chains";
            parentName = debris ? "NoteDebrisMesh" : "NoteCube";
            break;
        case NoteType::ChainLink:
            childName = debris ? "ChainLinkDebris" : mirror ? "MirrorChains" : "Chains";
            parentName = debris ? "NoteDebrisMesh" : "NoteCube";
            break;
    }

    auto parent = prefab->transform->Find(parentName);
    if (!parent) {
        logger.error("failed to find expected object {} on {}", parentName, prefab->name);
        return;
    }

    // would apply size here, but it gets overridden by NoteController::Init, so it's done in a hook instead

    Note* note = dynamic_cast<Note*>(assets[Selection::Note]);
    if (!note->asset.asset)
        return;  // default model

    if (modifiers && (modifiers->ghostNotes || modifiers->disappearingArrows))
        return;  // don't replace these for integrity

    if (debris && getConfig().NotesSettings().defaultDebris || type == NoteType::Bomb && getConfig().NotesSettings().defaultBombs)
        return;  // default debris/bombs override

    auto instance = note->asset.InstantiateChild(childName);
    if (!instance)
        return;  // replacement not found

    AddLoadingGuard();

    if (debris)
        instance->AddComponent<DebrisVisuals*>();
    else if (type != NoteType::Bomb)
        instance->AddComponent<NoteVisuals*>();

    // disable default model
    parent->GetComponent<UnityEngine::MeshFilter*>()->mesh = nullptr;

    if (type != NoteType::Bomb && !debris && !note->info.showArrows) {
        for (auto& name : {"NoteArrow", "NoteArrowGlow", "Circle", "NoteCircleGlow"}) {
            if (auto child = parent->Find(name))
                child->gameObject->active = false;
        }
    }

    SetLayerRecursively(instance->transform, debris ? 9 : 8);
    ReplaceMaterials(instance);  // shouldn't this happen before setting the properties on the mirrored materials?
    instance->transform->SetParent(parent, false);
    instance->transform->localScale = {0.4, 0.4, 0.4};

    RemoveLoadingGuard();
}

static void ColorChildNotes(UnityEngine::Transform* parent, bool chain) {
    if (!parent)
        return;
    auto const& names = chain ? ChainChildren : NoteChildren;
    for (int i = 0; i < names.size(); i++) {
        auto child = parent->Find(names[i]);
        if (child)
            child->gameObject->AddComponent<CustomModels::ColorVisuals*>()->SetMenuColor(i % 2 == 0);
        else
            logger.warn("child {} not found", names[i]);
    }
}

static void ColorDefaultNotes(UnityEngine::Transform* parent, UnityEngine::Color leftColor, UnityEngine::Color rightColor) {
    for (auto visuals : parent->GetComponentsInChildren<GlobalNamespace::ColorNoteVisuals*>()) {
        auto color = visuals->name->Contains("Left") ? leftColor : rightColor;
        for (auto controller : visuals->_materialPropertyBlockControllers) {
            controller->materialPropertyBlock->SetColor(CustomModels::Material::Color, color);
            controller->ApplyChanges();
        }
    }
}

UnityEngine::Transform* CustomModels::PreviewNotes(UnityEngine::Vector3 position, UnityEngine::Quaternion rotation) {
    logger.debug("creating note preview");

    AddLoadingGuard();

    auto preview = UnityEngine::GameObject::New_ctor("CustomModelsPreview")->transform;

    Note* note = dynamic_cast<Note*>(assets[Selection::Note]);
    if (note->asset.asset) {
        auto parent = note->asset.Instantiate()->transform;
        parent->SetParent(preview, false);

        for (auto disable : {"MirrorNotes", "MirrorBomb", "MirrorChains", "Debris", "ChainHeadDebris", "ChainLinkDebris"}) {
            if (auto child = parent->Find(disable))
                UnityEngine::Object::DestroyImmediate(child->gameObject);
        }

        ColorChildNotes(parent->Find("Notes"), false);
        ColorChildNotes(parent->Find("Chains"), true);
        parent->localScale = {0.4, 0.4, 0.4};

        GetDefaultBomb()->transform->SetParent(preview, false);
    } else {
        auto parent = GetDefaultNotes()->transform;
        ColorDefaultNotes(parent, MenuLeftColor(), MenuRightColor());
        parent->SetParent(preview, false);
    }

    RemoveLoadingGuard();

    preview->SetPositionAndRotation(position, rotation);
    return preview;
}

static inline void SetPositionRelativeTo(UnityEngine::Transform* anchor, UnityEngine::Transform* child, UnityEngine::Vector2 pos) {
    if (child)
        child->position = anchor->TransformPoint(pos.x, pos.y, 0);
}

void CustomModels::UpdateNotesPreview(UnityEngine::Transform* preview) {
    AddLoadingGuard();

    auto& settings = getConfig().NotesSettings();
    auto parent = preview->GetChild(0);

    float distance = 0.6;
    float x = 0;

    if (auto notes = parent->Find("Notes")) {
        notes->localScale = UnityEngine::Vector3::get_one() * settings.Size();
        SetPositionRelativeTo(preview, notes->Find(NoteChildren[0]), {0, distance});
        SetPositionRelativeTo(preview, notes->Find(NoteChildren[1]), {distance, distance});
        SetPositionRelativeTo(preview, notes->Find(NoteChildren[2]), {0, 0});
        SetPositionRelativeTo(preview, notes->Find(NoteChildren[3]), {distance, 0});
        x += distance * 2;
    }

    if (auto chains = parent->Find("Chains")) {
        chains->localScale = UnityEngine::Vector3::get_one() * settings.Size();
        SetPositionRelativeTo(preview, chains->Find(ChainChildren[0]), {x, distance});
        SetPositionRelativeTo(preview, chains->Find(ChainChildren[1]), {x + distance, distance});
        SetPositionRelativeTo(preview, chains->Find(ChainChildren[2]), {x, 0});
        SetPositionRelativeTo(preview, chains->Find(ChainChildren[3]), {x + distance, 0});
        x += distance * 2;
    }

    auto bomb = parent->Find("Bomb");
    SetPositionRelativeTo(preview, bomb, {x, 0});

    // child of preview instead of parent to avoid the 0.4x scale for custom notes
    if (auto defaultBomb = preview->Find("Bomb")) {
        if (bomb)
            bomb->gameObject->active = !settings.defaultBombs;
        SetPositionRelativeTo(preview, defaultBomb, {x, 0});
        defaultBomb->gameObject->active = !bomb || settings.defaultBombs;
    }

    RemoveLoadingGuard();
}

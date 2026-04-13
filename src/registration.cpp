#include "registration.hpp"

#include "GlobalNamespace/ColorManager.hpp"
#include "Zenject/ConcreteIdBinderGeneric_1.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "json.hpp"
#include "lapiz/shared/sabers/SaberModelRegistration.hpp"
#include "main.hpp"
#include "material.hpp"
#include "note.hpp"
#include "saber.hpp"
#include "wall.hpp"
#include "zip.hpp"

DEFINE_TYPE(CustomModels, CustomSaberAPI);

bool CustomModels::CustomSaberAPI::InitOverride(UnityEngine::Transform* parent, GlobalNamespace::Saber* saber) {
    logger.debug("ingame saber init");

    if (transform->GetChildCount() > 0) {
        logger.debug("already have a child! will update colors but not create saber");
        UpdateSaberColor(transform, false, saber->saberType);
        return false;
    }

    transform->SetParent(parent, false);
    InitSaber(transform, false, saber->saberType);

    return false;
}

GlobalNamespace::GameplayModifiers* CustomModels::modifiers = nullptr;

static GlobalNamespace::GameNoteController* DecorateNote(GlobalNamespace::GameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::Note, false, false);
    return prefab;
}

static GlobalNamespace::BombNoteController* DecorateBomb(GlobalNamespace::BombNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::Bomb, false, false);
    return prefab;
}

static GlobalNamespace::GameNoteController* DecorateChainHead(GlobalNamespace::GameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainHead, false, false);
    return prefab;
}

static GlobalNamespace::BurstSliderGameNoteController* DecorateChainLink(GlobalNamespace::BurstSliderGameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainLink, false, false);
    return prefab;
}

static GlobalNamespace::NoteDebris* DecorateDebris(GlobalNamespace::NoteDebris* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::Note, true, false);
    return prefab;
}

static GlobalNamespace::NoteDebris* DecorateChainHeadDebris(GlobalNamespace::NoteDebris* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainHead, true, false);
    return prefab;
}

static GlobalNamespace::NoteDebris* DecorateChainLinkDebris(GlobalNamespace::NoteDebris* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainLink, true, false);
    return prefab;
}

static GlobalNamespace::MirroredGameNoteController* DecorateMirrorNote(GlobalNamespace::MirroredGameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::Note, false, true);
    return prefab;
}

static GlobalNamespace::MirroredBombNoteController* DecorateMirrorBomb(GlobalNamespace::MirroredBombNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::Bomb, false, true);
    return prefab;
}

static GlobalNamespace::MirroredGameNoteController* DecorateMirrorChainHead(GlobalNamespace::MirroredGameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainHead, false, true);
    return prefab;
}

static GlobalNamespace::MirroredGameNoteController* DecorateMirrorChainLink(GlobalNamespace::MirroredGameNoteController* prefab) {
    CustomModels::InitNote(prefab->gameObject, CustomModels::NoteType::ChainLink, false, true);
    return prefab;
}

static GlobalNamespace::ObstacleController* DecorateObstacle(GlobalNamespace::ObstacleController* prefab) {
    CustomModels::InitWall(prefab->gameObject, false);
    return prefab;
}

static GlobalNamespace::MirroredObstacleController* DecorateMirrorObstacle(GlobalNamespace::MirroredObstacleController* prefab) {
    CustomModels::InitWall(prefab->gameObject, true);
    return prefab;
}

#define REGISTER_REDECORATOR(name, lapizName) \
    Lapiz::Objects::Beatmap::lapizName##Registration::New_ctor(Decorate##name, Priority)->RegisterRedecorator(container)

void CustomModels::Register(Zenject::DiContainer* container) {
    if (!getConfig().Enabled.GetValue())
        return;

    logger.info("registering sabers");

    auto reg = Lapiz::Sabers::SaberModelRegistration::Create<CustomSaberAPI*>(Priority);
    container->Bind<Lapiz::Sabers::SaberModelRegistrationWrapper*>()
        ->FromInstance(Lapiz::Sabers::SaberModelRegistrationWrapper::Make(reg))
        ->AsSingle();

    logger.info("registering notes");

    REGISTER_REDECORATOR(Note, BasicNote);
    REGISTER_REDECORATOR(Note, ProModeNote);
    REGISTER_REDECORATOR(Bomb, BombNote);
    REGISTER_REDECORATOR(ChainHead, BurstSliderHeadNote);
    REGISTER_REDECORATOR(ChainLink, BurstSliderNote);
    REGISTER_REDECORATOR(Debris, NoteDebrisHD);
    REGISTER_REDECORATOR(ChainHeadDebris, ChainHeadDebrisHD);
    REGISTER_REDECORATOR(ChainLinkDebris, ChainLinkDebrisHD);
    REGISTER_REDECORATOR(Debris, NoteDebrisLW);
    REGISTER_REDECORATOR(ChainHeadDebris, ChainHeadDebrisLW);
    REGISTER_REDECORATOR(ChainLinkDebris, ChainLinkDebrisLW);
    REGISTER_REDECORATOR(MirrorNote, MirrorNote);
    REGISTER_REDECORATOR(MirrorBomb, MirrorBomb);
    REGISTER_REDECORATOR(MirrorChainHead, MirrorChainHead);
    REGISTER_REDECORATOR(MirrorChainLink, MirrorChainLink);

    logger.info("registering walls");

    REGISTER_REDECORATOR(Obstacle, Obstacle);
    REGISTER_REDECORATOR(MirrorObstacle, MirrorObstacle);

    ClearMaterialsCache();
}

#include "legacy.hpp"

#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "json.hpp"
#include "main.hpp"
#include "metacore/shared/strings.hpp"
#include "zip.hpp"

static void ExtractZipFiles(std::filesystem::path destination) {
    std::filesystem::path folder = getDataDir(MOD_ID);
    folder /= "Zips";

    logger.info("extracting legacy zip files");

    std::vector<std::filesystem::path> remove;

    for (auto const& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_directory() || entry.path().extension() != ".zip") {
            logger.debug("skipping file {}", entry.path().string());
            continue;
        }
        if (CustomModels::ExtractAllLegacyAssets(entry.path(), destination))
            remove.emplace_back(entry.path());
    }

    for (auto& file : remove)
        std::filesystem::remove(file);
}

static bool LoadManifest(UnityEngine::AssetBundle* bundle, CustomModels::Manifest& manifest, std::string const& file) {
    manifest.androidFileName = "model.bundle";

    auto text = bundle->LoadAsset<UnityEngine::TextAsset*>("descriptor");
    if (!text) {
        logger.warn("legacy config not found");
        manifest.descriptor.author = "- Unknown -";
        manifest.descriptor.objectName = file;
        manifest.descriptor.description = "- Legacy file -";
        return true;
    }
    std::string json = text->text;
    CustomModels::LegacyDescriptor legacy;
    try {
        ReadFromString(json, legacy);
    } catch (std::exception const& e) {
        logger.error("failed to parse legacy descriptor: {}", e.what());
        logger.debug("descriptor was {}", json);
        return false;
    }
    manifest.descriptor.author = legacy.authorName;
    manifest.descriptor.objectName = legacy.objectName;
    manifest.descriptor.description = legacy.description;
    return true;
}

static void
LoadThumbnail(UnityEngine::AssetBundle* bundle, CustomModels::Descriptor& descriptor, std::vector<std::pair<std::string, std::string>>& files) {
    auto thumbnail = bundle->LoadAsset<UnityEngine::Texture2D*>("thumbnail");
    if (!thumbnail)
        return;
    descriptor.coverImage = "thumbnail.png";
    auto bytes = UnityEngine::ImageConversion::EncodeToPNG(thumbnail);
    files.emplace_back(descriptor.coverImage, std::string(bytes.begin(), bytes.end()));
}

static void WriteZip(
    UnityEngine::AssetBundle* bundle,
    CustomModels::Manifest& manifest,
    std::filesystem::path const& bundlePath,
    std::filesystem::path const& folder,
    std::filesystem::path const& file
) {
    std::vector<std::pair<std::string, std::string>> files;

    LoadThumbnail(bundle, manifest.descriptor, files);
    files.emplace_back(manifest.androidFileName, readfile(bundlePath.string()));
    files.emplace_back("package.json", WriteToString(manifest));

    std::string name = MetaCore::Strings::UniqueFileName(file, folder);
    CustomModels::WriteZipFile(folder / name, files);

    logger.info("converted {} to {}", bundlePath.filename().string(), name);
}

template <class T>
static T TryLoadLegacyConfig(UnityEngine::AssetBundle* bundle, std::string const& type) {
    auto text = bundle->LoadAsset<UnityEngine::TextAsset*>("config");
    if (!text) {
        logger.warn("legacy {} config not found", type);
        return {};
    }
    std::string json = text->text;
    try {
        return ReadFromString<T>(json);
    } catch (std::exception const& e) {
        logger.error("failed to parse legacy {} config: {}", type, e.what());
        logger.debug("config was {}", json);
    }
    return {};
}

static CustomModels::SaberInfo LoadSaberConfig(UnityEngine::AssetBundle* bundle) {
    auto legacy = TryLoadLegacyConfig<CustomModels::LegacySaberConfig>(bundle, "saber");
    CustomModels::SaberInfo config;
    config.hasTrail = legacy.hasCustomTrails;
    config.keepFakeGlow = legacy.enableFakeGlow;
    config.isLegacy = true;
    return config;
}

static bool ConvertQSaber(std::filesystem::path const& path) {
    std::filesystem::path folder = getDataDir(MOD_ID);
    folder /= "Sabers";

    logger.info("converting {}", path.string());

    auto output = path.stem().string() + ".whacker";

    auto bundle = UnityEngine::AssetBundle::LoadFromFile(path.string());
    if (!bundle)
        return false;

    CustomModels::Manifest manifest;
    if (!LoadManifest(bundle, manifest, path.stem())) {
        bundle->Unload(true);
        return false;
    }
    manifest.config = LoadSaberConfig(bundle);

    WriteZip(bundle, manifest, path, folder, output);
    bundle->Unload(true);
    return true;
}

static CustomModels::NoteInfo LoadNoteConfig(UnityEngine::AssetBundle* bundle) {
    auto legacy = TryLoadLegacyConfig<CustomModels::LegacyNoteConfig>(bundle, "note");
    CustomModels::NoteInfo config;
    config.hasDebris = legacy.hasDebris;
    config.hasBomb = legacy.hasBomb;
    config.showArrows = !legacy.disableBaseGameArrows;
    config.isLegacy = true;
    return config;
}

static bool ConvertQBloq(std::filesystem::path const& path) {
    std::filesystem::path folder = getDataDir(MOD_ID);
    folder /= "Notes";

    logger.info("converting {}", path.string());

    auto output = path.stem().string() + ".cyoob";

    auto bundle = UnityEngine::AssetBundle::LoadFromFile(path.string());
    if (!bundle)
        return false;

    CustomModels::Manifest manifest;
    if (!LoadManifest(bundle, manifest, path.stem())) {
        bundle->Unload(true);
        return false;
    }
    manifest.config = LoadNoteConfig(bundle);

    WriteZip(bundle, manifest, path, folder, output);
    bundle->Unload(true);
    return true;
}

static CustomModels::WallInfo LoadWallConfig(UnityEngine::AssetBundle* bundle) {
    auto legacy = TryLoadLegacyConfig<CustomModels::LegacyWallConfig>(bundle, "wall");
    CustomModels::WallInfo config;
    config.replaceCoreMaterial = legacy.replaceCoreMaterial;
    config.replaceFrameMaterial = legacy.replaceFrameMaterial;
    config.replaceCoreMesh = legacy.replaceCoreMesh;
    config.replaceFrameMesh = legacy.replaceFrameMesh;
    config.disableFrame = legacy.disableFrame;
    config.disableFakeGlow = legacy.disableFakeGlow;
    config.isLegacy = true;
    return config;
}

static bool ConvertQWall(std::filesystem::path const& path) {
    std::filesystem::path folder = getDataDir(MOD_ID);
    folder /= "Walls";

    logger.info("converting {}", path.string());

    auto output = path.stem().string() + ".box";

    auto bundle = UnityEngine::AssetBundle::LoadFromFile(path.string());
    if (!bundle)
        return false;

    CustomModels::Manifest manifest;
    if (!LoadManifest(bundle, manifest, path.stem())) {
        bundle->Unload(true);
        return false;
    }
    manifest.config = LoadWallConfig(bundle);

    WriteZip(bundle, manifest, path, folder, output);
    bundle->Unload(true);
    return true;
}

void CustomModels::ConvertLegacyModels() {
    std::filesystem::path folder = getDataDir(MOD_ID);
    folder /= "Legacy";

    ExtractZipFiles(folder);

    logger.info("converting legacy models");

    std::vector<std::filesystem::path> remove;

    for (auto const& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_directory())
            continue;
        try {
            std::string extension = entry.path().extension();
            bool converted = false;
            if (extension == ".qsaber")
                converted = ConvertQSaber(entry.path());
            else if (extension == ".qbloq")
                converted = ConvertQBloq(entry.path());
            else if (extension == ".qwall")
                converted = ConvertQWall(entry.path());
            else
                logger.debug("skipping file {}", entry.path().string());
            if (converted)
                remove.emplace_back(entry.path());
        } catch (il2cpp_utils::exceptions::StackTraceException const& exc) {
            logger.error("failed to convert legacy model: {}", exc.what());
        } catch (...) {
            logger.error("failed to convert legacy model: unknown error");
        }
    }

    for (auto& file : remove)
        std::filesystem::remove(file);
}

static void CopyFolder(std::string const& source, std::string const& subfolder) {
    if (!direxists(source))
        return;

    std::filesystem::path folder = getDataDir(MOD_ID);
    auto dest = folder / subfolder;

    logger.info("moving models from {} to {}", source, dest.string());

    for (auto const& entry : std::filesystem::directory_iterator(source)) {
        if (entry.is_directory())
            continue;
        std::error_code ignore;
        std::filesystem::rename(entry, dest / entry.path().filename(), ignore);
    }
}

void CustomModels::MoveQosmeticsFolders() {
    std::filesystem::path qos = getDataDir("Qosmetics");
    CopyFolder(qos / "Whackers", "Sabers");
    CopyFolder(qos / "Cyoobs", "Notes");
    CopyFolder(qos / "Boxes", "Walls");
}

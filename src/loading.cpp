#include "loading.hpp"

#include "UnityEngine/AssetBundleCreateRequest.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "main.hpp"
#include "metacore/shared/operators.hpp"
#include "utils.hpp"
#include "zip.hpp"

UnityEngine::GameObject* CustomModels::Asset::GetChild(std::string const& name) {
    if (!asset)
        return nullptr;
    if (auto child = asset->transform->Find(name))
        return child->gameObject;
    return nullptr;
}

UnityEngine::GameObject* CustomModels::Asset::InstantiateChild(std::string const& name) {
    if (auto child = GetChild(name))
        return CustomModels::Instantiate(child);
    return nullptr;
}

UnityEngine::GameObject* CustomModels::Asset::Instantiate() {
    if (!asset)
        return nullptr;
    return CustomModels::Instantiate(asset);
}

struct BundleReference {
    int refs = 0;
    UnityEngine::AssetBundle* bundle = nullptr;
    std::vector<std::function<void(UnityEngine::AssetBundle*)>> callbacks = {};
};

static std::map<std::string, BundleReference> bundles;

// removes a reference from an asset, handling any necessary cleanup, returns true if the file is no longer in bundles
static bool RemoveLoad(std::string const& file) {
    auto& bundle = bundles[file];
    if (--bundle.refs != 0)
        return false;
    logger.debug("unloading asset bundle {}", file);
    if (bundle.bundle)
        bundle.bundle->Unload(true);
    bundles.erase(file);
    return true;
}

static void LoadBundle(std::string const& bytes, std::string file) {
    static auto LoadFromMemoryAsync = il2cpp_utils::resolve_icall<UnityEngine::AssetBundleCreateRequest*, ArrayW<uint8_t>, uint>(
        "UnityEngine.AssetBundle::LoadFromMemoryAsync_Internal"
    );

    // keep a reference while loading in case it's cancelled partway through
    bundles[file].refs++;

    logger.debug("loading asset bundle {}", file);
    auto array = ArrayW<uint8_t>({(uint8_t*) bytes.data(), bytes.size()});
    auto request = LoadFromMemoryAsync(array, 0);

    BSML::MainThreadScheduler::ScheduleUntil(
        [request]() { return request->isDone; },
        [request = SafePtr(request), file]() {
            auto& bundle = bundles[file];
            auto loaded = request->assetBundle;
            bundle.bundle = loaded;
            decltype(bundle.callbacks) callbacks;
            callbacks.swap(bundle.callbacks);
            RemoveLoad(file);
            for (auto& callback : callbacks)
                callback(loaded);
        }
    );
}

// loads a file, reusing reference if already loading or loaded
static void AddLoad(std::string const& file, std::function<void(UnityEngine::AssetBundle*)> onDone) {
    bool present = bundles.contains(file);
    logger.debug("adding reference for asset {}, will load: {}", file, !present);

    auto& bundle = bundles[file];
    bundle.refs++;
    if (bundle.bundle)
        onDone(bundle.bundle);
    else
        bundle.callbacks.emplace_back(onDone);

    if (!present) {
        std::string bytes = CustomModels::ReadFileFromZip(file, CustomModels::files[file].androidFileName);
        if (!bytes.empty())
            LoadBundle(bytes, file);
        else
            onDone(nullptr);
    }
}

void CustomModels::Asset::Load(std::string file, std::string assetName, std::function<void(bool, bool)> onDone) {
    if (!files.contains(file) || file.empty() || file == currentFile) {
        onDone(false, !files.contains(file) || file.empty());
        return;
    }

    logger.info("getting asset {} for {}", assetName, file);
    Unload();
    loadingFile = file;

    // nesting callbacks is definitely annoying, but a coroutine would be too
    AddLoad(file, [this, file, assetName, onDone](UnityEngine::AssetBundle* bundle) {
        if (file != loadingFile) {
            onDone(false, false);
            return;
        }
        if (!bundle) {
            logger.error("failed to load asset bundle for {}", file);
            loadingFile = "";
            RemoveLoad(file);
            onDone(false, true);
            return;
        }

        logger.debug("loading asset {} from {}", assetName, file);
        auto request = bundle->LoadAssetAsync<UnityEngine::GameObject*>(assetName);

        BSML::MainThreadScheduler::ScheduleUntil(
            [request]() { return request->isDone; },
            [this, bundle, request = SafePtr(request), assetName, file, onDone]() {
                if (file != loadingFile) {
                    onDone(false, false);
                    return;
                }

                logger.debug("finished loading asset {} for {}", assetName, file);
                currentFile = file;
                loadingFile = "";

                this->bundle = bundle;
                if (auto object = request->asset.try_cast<UnityEngine::GameObject>()) {
                    asset = object->ptr();
                    asset->hideFlags = asset->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;
                } else
                    logger.error("failed to load asset {} for {}", assetName, file);

                onDone(true, asset == nullptr);
            }
        );
    });
}

void CustomModels::Asset::Unload() {
    logger.info("removing reference to {} (and {})", currentFile, loadingFile);
    if (!loadingFile.empty())
        RemoveLoad(loadingFile);
    if (!currentFile.empty())
        RemoveLoad(currentFile);
    loadingFile = "";
    currentFile = "";
    bundle = nullptr;
    asset = nullptr;
}

std::map<std::string, CustomModels::Manifest> CustomModels::files;

static std::set<std::string> const extensions = {".whacker", ".cyoob", ".box"};

static void LoadManifest(std::string const& file) {
    logger.debug("loading manifest for {}", file);
    auto package = CustomModels::ReadFileFromZip(file, "package.json");
    if (package.empty()) {
        logger.error("failed to read package.json from {}", file);
        return;
    }

    CustomModels::Manifest manifest;
    try {
        ReadFromString(package, manifest);
    } catch (std::exception const& e) {
        logger.error("failed to parse package.json from {}: {}", file, e.what());
        return;
    }
    CustomModels::files[file] = manifest;
}

static void LoadDirectory(std::string const& directory) {
    logger.debug("loading manifests in {}", directory);

    for (auto const& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_directory())
            continue;
        auto path = entry.path();
        if (extensions.contains(path.extension()))
            LoadManifest(path);
        else
            logger.debug("skipping loading manifest for {} due to invalid extension {}", path.string(), path.extension().string());
    }
}

void CustomModels::LoadManifests() {
    logger.info("refreshing manifests");
    files.clear();

    std::filesystem::path folder = getDataDir(MOD_ID);
    for (auto& subfolder : {"Sabers", "Notes", "Walls"})
        LoadDirectory(folder / subfolder);
}

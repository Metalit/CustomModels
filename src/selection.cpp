#include "selection.hpp"

#include "UnityEngine/HideFlags.hpp"
#include "assets.hpp"
#include "config.hpp"
#include "main.hpp"
#include "metacore/shared/operators.hpp"
#include "metacore/shared/strings.hpp"
#include "note.hpp"
#include "saber.hpp"
#include "settings.hpp"
#include "wall.hpp"
#include "zip.hpp"

void CustomModels::AssetInfo::Load(std::string const& file, std::function<void()> onDone) {
    if (!file.empty() && !getConfig().LoadFailed(file)) {
        try {
            if (!files.contains(file))
                throw std::runtime_error("manifest not found");
            if (!ParseInfo(file))
                throw std::runtime_error("failed to parse info");
            asset.Load(file, ObjectName(), [this, onDone](bool changed, bool error) {
                if (error)
                    SetDefault();
                else if (changed) {
                    getConfig().LoadingGuard.SetValue(asset.currentFile);
                    PostLoad();
                    getConfig().LoadingGuard.SetValue("");
                }
                if (onDone)
                    onDone();
            });
            return;
        } catch (std::exception const& e) {
            logger.error("error loading file {}: {}", file, e.what());
        }
    }
    SetDefault();
    if (onDone)
        onDone();
}

static CustomModels::Saber SaberAsset;
static CustomModels::Saber TrailAsset;
static CustomModels::Saber MenuSaberAsset;
static CustomModels::Saber MenuTrailAsset;
static CustomModels::Note NoteAsset;
static CustomModels::Wall WallAsset;

std::map<CustomModels::Selection, CustomModels::AssetInfo*> CustomModels::assets = {
    {CustomModels::Selection::Saber, &SaberAsset},
    {CustomModels::Selection::Trail, &TrailAsset},
    {CustomModels::Selection::MenuSaber, &MenuSaberAsset},
    {CustomModels::Selection::MenuTrail, &MenuTrailAsset},
    {CustomModels::Selection::Note, &NoteAsset},
    {CustomModels::Selection::Wall, &WallAsset},
};

void CustomModels::LoadSelections(std::function<void()> onDone) {
    int* counter = nullptr;
    std::function<void()> count = nullptr;
    if (onDone) {
        counter = new int(0);  // callbacks are all run on the main thread
        count = [counter, onDone]() {
            if (++(*counter) == 6) {
                logger.debug("finished loading all selections");
                delete counter;
                onDone();
            }
        };
    }
    assets[Selection::Saber]->Load(getConfig().SaberModel(), count);
    assets[Selection::Trail]->Load(getConfig().TrailModel(), count);
    assets[Selection::MenuSaber]->Load(getConfig().MenuSaberModel(), count);
    assets[Selection::MenuTrail]->Load(getConfig().MenuTrailModel(), count);
    assets[Selection::Note]->Load(getConfig().NoteModel(), count);
    assets[Selection::Wall]->Load(getConfig().WallModel(), count);
}

#define ICON_GETTER(name, asset)                                                                   \
    static UnityEngine::Sprite* name() {                                                           \
        static UnityEngine::Sprite* sprite = nullptr;                                              \
        if (!sprite) {                                                                             \
            sprite = PNG_SPRITE(icons::asset);                                                     \
            sprite->hideFlags = sprite->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset; \
        }                                                                                          \
        return sprite;                                                                             \
    }

ICON_GETTER(CopyTrail, custom_trail);
ICON_GETTER(NoTrail, no_trail);
ICON_GETTER(NoSaber, no_saber);
ICON_GETTER(DefaultSabers, default_sabers);
ICON_GETTER(DefaultNotes, default_notes);
ICON_GETTER(DefaultWalls, default_walls);
ICON_GETTER(EmptyPreview, null_image);

static std::string& GetModelConfig(int selection) {
    switch ((CustomModels::Selection) selection) {
        case CustomModels::Selection::Saber:
            return getConfig().SaberModel();
        case CustomModels::Selection::Trail:
            return getConfig().TrailModel();
        case CustomModels::Selection::MenuSaber:
            return getConfig().MenuSaberModel();
        case CustomModels::Selection::MenuTrail:
            return getConfig().MenuTrailModel();
        case CustomModels::Selection::Note:
            return getConfig().NoteModel();
        default:
            return getConfig().WallModel();
    }
}

static std::string& GetCurrentModelConfig() {
    return GetModelConfig(CustomModels::SettingsCoordinator::ModelSelection());
}

static int& GetTrailConfig() {
    bool menu = CustomModels::SettingsCoordinator::GetInstance()->menuPointer;
    return menu ? getConfig().MenuTrailMode() : getConfig().TrailMode();
}

struct TrailModeListItem : CustomModels::ListItem {
    int mode;
    TrailModeListItem(int mode) : mode(mode) {}

    UnityEngine::Sprite* Cover() override { return mode == 0 ? CopyTrail() : NoTrail(); }
    std::string Name() override { return mode == 0 ? "Use Saber Trail" : "No Trail"; }
    std::string Author() override { return ""; }
    void Select(std::function<void()> onLoaded) override {
        bool menu = CustomModels::SettingsCoordinator::GetInstance()->menuPointer;
        CustomModels::assets[menu ? CustomModels::Selection::MenuTrail : CustomModels::Selection::Trail]->SetDefault();
        GetCurrentModelConfig() = "";
        GetTrailConfig() = mode;
        getConfig().Save();
        onLoaded();
    }
    bool Selected() override { return GetTrailConfig() == mode; }
};

struct NoMenuSaberListItem : CustomModels::ListItem {
    UnityEngine::Sprite* Cover() override { return NoSaber(); }
    std::string Name() override { return "No Menu Pointer"; }
    std::string Author() override { return ""; }
    void Select(std::function<void()> onLoaded) override {
        CustomModels::assets[CustomModels::Selection::MenuSaber]->SetDefault();
        getConfig().MenuSaberModel() = "";
        getConfig().MenuSaber() = false;
        getConfig().Save();
        onLoaded();
    }
    bool Selected() override { return !getConfig().MenuSaber(); }
};

struct DefaultListItem : CustomModels::ListItem {
    int modelType;
    DefaultListItem(int modelType) : modelType(modelType) {}

    UnityEngine::Sprite* Cover() override {
        switch (modelType) {
            case 0:
                return DefaultSabers();
            case 1:
                return DefaultNotes();
            default:
                return DefaultWalls();
        }
    }
    std::string Name() override {
        switch (modelType) {
            case 0:
                return "Default Sabers";
            case 1:
                return "Default Notes";
            default:
                return "Default Walls";
        }
    }
    std::string Author() override { return "Beat Games"; }
    void Select(std::function<void()> onLoaded) override {
        GetCurrentModelConfig() = "";
        if (modelType == 0) {
            auto settings = CustomModels::SettingsCoordinator::GetInstance();
            if (settings->trail)
                GetTrailConfig() = 2;
            else if (settings->menuPointer)
                getConfig().MenuSaber() = true;
        }
        getConfig().Save();
        auto selection = (CustomModels::Selection) CustomModels::SettingsCoordinator::ModelSelection();
        CustomModels::assets[selection]->SetDefault();
        onLoaded();
    }
    bool Selected() override { return GetCurrentModelConfig().empty(); }
};

static std::map<std::string, UnityEngine::Sprite*> fileCovers;

static UnityEngine::Sprite* GetCover(std::string const& file) {
    if (fileCovers.contains(file))
        return fileCovers[file];
    if (!CustomModels::files.contains(file))
        return nullptr;
    auto const& image = CustomModels::files[file].descriptor.coverImage;
    logger.debug("loading cover for {}", file);
    if (image.empty())
        return nullptr;
    std::string bytes = CustomModels::ReadFileFromZip(file, image);
    auto sprite = BSML::Utilities::LoadSpriteRaw(ArrayW<uint8_t>({(uint8_t*) bytes.data(), bytes.size()}));
    if (!sprite)
        return nullptr;
    sprite->hideFlags = sprite->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;
    fileCovers[file] = sprite;
    return sprite;
}

static std::set<std::string> StoredCovers() {
    std::set<std::string> ret;
    for (auto const& pair : fileCovers)
        ret.emplace(pair.first);
    return ret;
}

struct FileListItem : CustomModels::ListItem {
    std::string file;
    FileListItem(std::string file) : file(std::move(file)) {}

    UnityEngine::Sprite* Cover() override {
        if (auto cover = GetCover(file))
            return cover;
        return EmptyPreview();
    }
    std::string Name() override { return CustomModels::files[file].descriptor.objectName; }
    std::string Author() override { return CustomModels::files[file].descriptor.author; }
    void Select(std::function<void()> onLoaded) override {
        logger.debug("select model {}", file);
        auto selection = (CustomModels::Selection) CustomModels::SettingsCoordinator::ModelSelection();
        if (selection == CustomModels::Selection::Trail)
            getConfig().TrailMode() = 2;
        else if (selection == CustomModels::Selection::MenuTrail)
            getConfig().MenuTrailMode() = 2;
        else if (selection == CustomModels::Selection::MenuSaber)
            getConfig().MenuSaber() = true;
        GetCurrentModelConfig() = file;
        getConfig().Save();
        CustomModels::assets[selection]->Load(file, std::move(onLoaded));
    }
    bool Selected() override { return GetCurrentModelConfig() == file; }
    void Delete() override {
        logger.debug("delete model {}", file);
        bool changed = false;
        for (int i = 0; i <= (int) CustomModels::Selection::SelectionMax; i++) {
            auto& model = GetModelConfig(i);
            if (model == file) {
                model = "";
                changed = true;
                CustomModels::assets[(CustomModels::Selection) i]->SetDefault();
            }
        }
        auto fails = getConfig().LoadingFailures.GetValue();
        for (int i = 0; i < fails.size(); i++) {
            if (fails[i] == file) {
                fails.erase(fails.begin() + i);
                getConfig().LoadingFailures.SetValue(fails, false);
                changed = true;
                break;
            }
        }
        if (changed)
            getConfig().Save();
        std::filesystem::remove(file);
        CustomModels::LoadManifests();
    }
    bool Deletable() override { return true; }
    bool Errored() override { return getConfig().LoadFailed(file); }
};

bool CustomModels::ListItem::Matches(std::string const& search) {
    auto lower = MetaCore::Strings::Lower(search);
    for (auto const& str : {Name(), Author()}) {
        if (MetaCore::Strings::Lower(str).find(lower) != std::string::npos)
            return true;
    }
    return false;
}

static std::vector<CustomModels::ListItem*> listItems;
static int prevModelType;
static bool prevMenu;
static bool prevTrail;

static std::vector<std::string> const Extensions = {".whacker", ".cyoob", ".box"};

static void RefreshListItems(int modelType, bool trail, bool menu) {
    for (auto item : listItems)
        delete item;
    listItems.clear();

    if (modelType == 0) {
        if (trail) {
            listItems.emplace_back(new TrailModeListItem(0));
            listItems.emplace_back(new TrailModeListItem(1));
        } else if (menu)
            listItems.emplace_back(new NoMenuSaberListItem());
    }
    listItems.emplace_back(new DefaultListItem(modelType));
    auto unsorted = listItems.size();

    auto remove = StoredCovers();
    for (auto const& [file, _] : CustomModels::files) {
        if (!file.ends_with(Extensions[modelType]))
            continue;
        listItems.emplace_back(new FileListItem(file));
        remove.erase(file);
    }
    for (auto const& file : remove) {
        UnityEngine::Object::Destroy(fileCovers[file]->texture);
        UnityEngine::Object::Destroy(fileCovers[file]);
        fileCovers.erase(file);
    }

    std::sort(listItems.begin() + unsorted, listItems.end(), [](auto a, auto b) {
        if (a->Name() == b->Name())
            return a->Author() < b->Author();
        return a->Name() < b->Name();
    });

    prevModelType = modelType;
    prevTrail = trail;
    prevMenu = menu;
}

std::pair<std::vector<CustomModels::ListItem*>, int> CustomModels::GetSelectionOptions(std::string filter, bool forceRefresh) {
    std::vector<CustomModels::ListItem*> ret;

    auto instance = SettingsCoordinator::GetInstance();
    int modelType = instance->modelType;
    bool trail = instance->trail;
    bool menu = instance->menuPointer;

    if (forceRefresh || prevModelType != modelType || modelType == 0 && (prevTrail != trail || prevMenu != menu))
        RefreshListItems(modelType, trail, menu);

    int selected = -1;

    for (auto item : listItems) {
        if (!item->Matches(filter))
            continue;
        if (selected < 0 && item->Selected())
            selected = ret.size();
        ret.emplace_back(item);
    }
    return {ret, selected};
}

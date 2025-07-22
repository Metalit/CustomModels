#pragma once

#include "UnityEngine/Sprite.hpp"
#include "loading.hpp"

namespace CustomModels {
    enum class Selection {
        Saber,
        Trail,
        MenuSaber,
        MenuTrail,
        Note,
        Wall,
        SelectionMax = Wall,
    };

    struct AssetInfo {
        Asset asset;

        virtual void SetDefault() = 0;
        virtual bool ParseInfo(std::string const& file) = 0;
        virtual std::string ObjectName() = 0;
        virtual void PostLoad() = 0;

        void Load(std::string const& file, std::function<void()> onDone);
    };

    extern std::map<Selection, AssetInfo*> assets;

    void LoadSelections(std::function<void()> onDone = nullptr);

    struct ListItem {
        virtual UnityEngine::Sprite* Cover() = 0;
        virtual std::string Name() = 0;
        virtual std::string Author() = 0;
        virtual void Select(std::function<void()> onLoaded) = 0;
        virtual bool Selected() = 0;
        virtual void Delete() {};
        virtual bool Deletable() { return false; };
        virtual bool Errored() { return false; };
        virtual ~ListItem() = default;

        bool Matches(std::string const& search);
    };

    std::pair<std::vector<ListItem*>, int> GetSelectionOptions(std::string filter, bool forceRefresh);
}

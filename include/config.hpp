#pragma once

#include "config-utils/shared/config-utils.hpp"

namespace CustomModels {
    DECLARE_JSON_STRUCT(SaberSettings) {
        VALUE_DEFAULT(float, length, 1);
        VALUE_DEFAULT(bool, overrideLength, false);
        VALUE_DEFAULT(float, width, 1);
        VALUE_DEFAULT(bool, overrideWidth, false);

        float Length() {
            return overrideLength ? length : 1;
        }
        float Width() {
            return overrideWidth ? width : 1;
        }
    };

    DECLARE_JSON_STRUCT(TrailSettings) {
        VALUE_DEFAULT(bool, whiteStep, false);
        VALUE_DEFAULT(float, length, 1);
        VALUE_DEFAULT(bool, overrideLength, false);
        VALUE_DEFAULT(float, width, 1);
        VALUE_DEFAULT(bool, overrideWidth, false);
        VALUE_DEFAULT(float, widthOffset, 0);

        float Length() {
            return overrideLength ? length : 1;
        }
        float Width() {
            return overrideWidth ? width : 1;
        }
    };

    DECLARE_JSON_STRUCT(NoteSettings) {
        VALUE_DEFAULT(float, size, 1);
        VALUE_DEFAULT(bool, overrideSize, false);
        VALUE_DEFAULT(bool, defaultBombs, false);
        VALUE_DEFAULT(bool, defaultDebris, false);

        float Size() {
            return overrideSize ? size : 1;
        }
    };

    DECLARE_JSON_STRUCT(WallSettings) {
        VALUE_DEFAULT(bool, disableFrame, false);
        VALUE_DEFAULT(bool, disableCore, false);
    };

    DECLARE_JSON_STRUCT(Profile) {
        // empty string is default model
        VALUE_DEFAULT(std::string, SaberModel, "");
        VALUE_DEFAULT(std::string, TrailModel, "");
        VALUE_DEFAULT(std::string, MenuSaberModel, "");
        VALUE_DEFAULT(std::string, MenuTrailModel, "");
        VALUE_DEFAULT(std::string, NoteModel, "");
        VALUE_DEFAULT(std::string, WallModel, "");

        VALUE_DEFAULT(bool, MenuSaber, false);
        // copy, none, custom
        VALUE_DEFAULT(int, TrailMode, 0);
        VALUE_DEFAULT(int, MenuTrailMode, 0);

        VALUE_DEFAULT(CustomModels::SaberSettings, SaberSettings, {});
        VALUE_DEFAULT(CustomModels::TrailSettings, TrailSettings, {});
        VALUE_DEFAULT(CustomModels::SaberSettings, MenuSaberSettings, {});
        VALUE_DEFAULT(CustomModels::TrailSettings, MenuTrailSettings, {});
        VALUE_DEFAULT(CustomModels::NoteSettings, NotesSettings, {});
        VALUE_DEFAULT(CustomModels::WallSettings, WallsSettings, {});
    };
}

#define PROFILE_GETTER(name) \
    auto& name() { return CurrentProfile().name; }

DECLARE_CONFIG(Config) {
    CONFIG_VALUE(Enabled, bool, "ModEnabled", true);

    CONFIG_VALUE(Profile, std::string, "Profile", "Default");

    MAP_DEFAULT(CustomModels::Profile, Profiles, StringKeyedMap<CustomModels::Profile>({{"Default", {}}}));

    CustomModels::Profile& CurrentProfile() {
        if (!Profiles.contains(Profile.GetValue()))
            Profile.SetValue(Profile.GetDefaultValue());
        return Profiles[Profile.GetValue()];
    }

    PROFILE_GETTER(SaberModel);
    PROFILE_GETTER(TrailModel);
    PROFILE_GETTER(MenuSaberModel);
    PROFILE_GETTER(MenuTrailModel);
    PROFILE_GETTER(NoteModel);
    PROFILE_GETTER(WallModel);

    PROFILE_GETTER(MenuSaber);
    PROFILE_GETTER(TrailMode);
    PROFILE_GETTER(MenuTrailMode);

    PROFILE_GETTER(SaberSettings);
    PROFILE_GETTER(TrailSettings);
    PROFILE_GETTER(MenuSaberSettings);
    PROFILE_GETTER(MenuTrailSettings);
    PROFILE_GETTER(NotesSettings);
    PROFILE_GETTER(WallsSettings);
};

#undef PROFILE_GETTER

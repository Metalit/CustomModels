#pragma once

#include "metacore/shared/assets.hpp"

#define DECLARE_ASSET(name, binary)       \
    const IncludedAsset name {            \
        Externs::_binary_##binary##_start, \
        Externs::_binary_##binary##_end    \
    };

#define DECLARE_ASSET_NS(namespaze, name, binary) \
    namespace namespaze { DECLARE_ASSET(name, binary) }

namespace IncludedAssets {
    namespace Externs {
        extern "C" uint8_t _binary_bsml_preview_bsml_start[];
        extern "C" uint8_t _binary_bsml_preview_bsml_end[];
        extern "C" uint8_t _binary_bsml_selection_bsml_start[];
        extern "C" uint8_t _binary_bsml_selection_bsml_end[];
        extern "C" uint8_t _binary_bsml_settings_bsml_start[];
        extern "C" uint8_t _binary_bsml_settings_bsml_end[];
        extern "C" uint8_t _binary_icons_bomb_png_start[];
        extern "C" uint8_t _binary_icons_bomb_png_end[];
        extern "C" uint8_t _binary_icons_copy_png_start[];
        extern "C" uint8_t _binary_icons_copy_png_end[];
        extern "C" uint8_t _binary_icons_custom_trail_png_start[];
        extern "C" uint8_t _binary_icons_custom_trail_png_end[];
        extern "C" uint8_t _binary_icons_debris_png_start[];
        extern "C" uint8_t _binary_icons_debris_png_end[];
        extern "C" uint8_t _binary_icons_default_notes_png_start[];
        extern "C" uint8_t _binary_icons_default_notes_png_end[];
        extern "C" uint8_t _binary_icons_default_sabers_png_start[];
        extern "C" uint8_t _binary_icons_default_sabers_png_end[];
        extern "C" uint8_t _binary_icons_default_walls_png_start[];
        extern "C" uint8_t _binary_icons_default_walls_png_end[];
        extern "C" uint8_t _binary_icons_delete_png_start[];
        extern "C" uint8_t _binary_icons_delete_png_end[];
        extern "C" uint8_t _binary_icons_edit_png_start[];
        extern "C" uint8_t _binary_icons_edit_png_end[];
        extern "C" uint8_t _binary_icons_no_core_png_start[];
        extern "C" uint8_t _binary_icons_no_core_png_end[];
        extern "C" uint8_t _binary_icons_no_frame_png_start[];
        extern "C" uint8_t _binary_icons_no_frame_png_end[];
        extern "C" uint8_t _binary_icons_no_saber_png_start[];
        extern "C" uint8_t _binary_icons_no_saber_png_end[];
        extern "C" uint8_t _binary_icons_no_trail_png_start[];
        extern "C" uint8_t _binary_icons_no_trail_png_end[];
        extern "C" uint8_t _binary_icons_note_png_start[];
        extern "C" uint8_t _binary_icons_note_png_end[];
        extern "C" uint8_t _binary_icons_note_size_png_start[];
        extern "C" uint8_t _binary_icons_note_size_png_end[];
        extern "C" uint8_t _binary_icons_null_image_png_start[];
        extern "C" uint8_t _binary_icons_null_image_png_end[];
        extern "C" uint8_t _binary_icons_saber_png_start[];
        extern "C" uint8_t _binary_icons_saber_png_end[];
        extern "C" uint8_t _binary_icons_saber_length_png_start[];
        extern "C" uint8_t _binary_icons_saber_length_png_end[];
        extern "C" uint8_t _binary_icons_saber_width_png_start[];
        extern "C" uint8_t _binary_icons_saber_width_png_end[];
        extern "C" uint8_t _binary_icons_trail_duration_png_start[];
        extern "C" uint8_t _binary_icons_trail_duration_png_end[];
        extern "C" uint8_t _binary_icons_trail_offset_png_start[];
        extern "C" uint8_t _binary_icons_trail_offset_png_end[];
        extern "C" uint8_t _binary_icons_trail_width_png_start[];
        extern "C" uint8_t _binary_icons_trail_width_png_end[];
        extern "C" uint8_t _binary_icons_wall_png_start[];
        extern "C" uint8_t _binary_icons_wall_png_end[];
    }

    // bsml/preview.bsml
    DECLARE_ASSET_NS(bsml, preview_bsml, bsml_preview_bsml);
    // bsml/selection.bsml
    DECLARE_ASSET_NS(bsml, selection_bsml, bsml_selection_bsml);
    // bsml/settings.bsml
    DECLARE_ASSET_NS(bsml, settings_bsml, bsml_settings_bsml);
    // icons/bomb.png
    DECLARE_ASSET_NS(icons, bomb_png, icons_bomb_png);
    // icons/copy.png
    DECLARE_ASSET_NS(icons, copy_png, icons_copy_png);
    // icons/custom_trail.png
    DECLARE_ASSET_NS(icons, custom_trail_png, icons_custom_trail_png);
    // icons/debris.png
    DECLARE_ASSET_NS(icons, debris_png, icons_debris_png);
    // icons/default_notes.png
    DECLARE_ASSET_NS(icons, default_notes_png, icons_default_notes_png);
    // icons/default_sabers.png
    DECLARE_ASSET_NS(icons, default_sabers_png, icons_default_sabers_png);
    // icons/default_walls.png
    DECLARE_ASSET_NS(icons, default_walls_png, icons_default_walls_png);
    // icons/delete.png
    DECLARE_ASSET_NS(icons, delete_png, icons_delete_png);
    // icons/edit.png
    DECLARE_ASSET_NS(icons, edit_png, icons_edit_png);
    // icons/no_core.png
    DECLARE_ASSET_NS(icons, no_core_png, icons_no_core_png);
    // icons/no_frame.png
    DECLARE_ASSET_NS(icons, no_frame_png, icons_no_frame_png);
    // icons/no_saber.png
    DECLARE_ASSET_NS(icons, no_saber_png, icons_no_saber_png);
    // icons/no_trail.png
    DECLARE_ASSET_NS(icons, no_trail_png, icons_no_trail_png);
    // icons/note.png
    DECLARE_ASSET_NS(icons, note_png, icons_note_png);
    // icons/note_size.png
    DECLARE_ASSET_NS(icons, note_size_png, icons_note_size_png);
    // icons/null_image.png
    DECLARE_ASSET_NS(icons, null_image_png, icons_null_image_png);
    // icons/saber.png
    DECLARE_ASSET_NS(icons, saber_png, icons_saber_png);
    // icons/saber_length.png
    DECLARE_ASSET_NS(icons, saber_length_png, icons_saber_length_png);
    // icons/saber_width.png
    DECLARE_ASSET_NS(icons, saber_width_png, icons_saber_width_png);
    // icons/trail_duration.png
    DECLARE_ASSET_NS(icons, trail_duration_png, icons_trail_duration_png);
    // icons/trail_offset.png
    DECLARE_ASSET_NS(icons, trail_offset_png, icons_trail_offset_png);
    // icons/trail_width.png
    DECLARE_ASSET_NS(icons, trail_width_png, icons_trail_width_png);
    // icons/wall.png
    DECLARE_ASSET_NS(icons, wall_png, icons_wall_png);
}

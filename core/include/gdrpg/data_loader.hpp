#pragma once

#include "gdrpg/registry.hpp"

#include <filesystem>
#include <string>

namespace gdrpg {

/// Loads JSON data files into a Registry.
class DataLoader {
public:
    /// Load all standard files from a directory:
    /// creatures.json, abilities.json, items.json, status_effects.json,
    /// and optionally persistent_entities.json.
    static bool load_directory(Registry& registry, const std::filesystem::path& dir,
                               std::string* error_out = nullptr);

    static bool load_abilities(Registry& registry, const std::filesystem::path& path,
                               std::string* error_out = nullptr);
    static bool load_creatures(Registry& registry, const std::filesystem::path& path,
                               std::string* error_out = nullptr);
    static bool load_items(Registry& registry, const std::filesystem::path& path,
                           std::string* error_out = nullptr);
    static bool load_status_effects(Registry& registry, const std::filesystem::path& path,
                                    std::string* error_out = nullptr);
    static bool load_persistent_entities(Registry& registry, const std::filesystem::path& path,
                                         std::string* error_out = nullptr);
};

} // namespace gdrpg

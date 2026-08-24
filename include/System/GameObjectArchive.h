#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class GameObject;

struct LoadedGameObjectArchive
{
    std::string name;
    std::vector<std::unique_ptr<GameObject>> gameObjects;

    LoadedGameObjectArchive() = default;
    ~LoadedGameObjectArchive();
    LoadedGameObjectArchive(LoadedGameObjectArchive&&) noexcept;
    LoadedGameObjectArchive& operator=(LoadedGameObjectArchive&&) noexcept;
    LoadedGameObjectArchive(const LoadedGameObjectArchive&) = delete;
    LoadedGameObjectArchive& operator=(const LoadedGameObjectArchive&) = delete;
};

// SceneとPrefabで共用するGameObject階層のYAML変換を担当する。
class GameObjectArchive final
{
public:
    static bool Save(
        const std::filesystem::path& path,
        std::string_view name,
        const std::vector<GameObject*>& gameObjects,
        bool includePrefabReferences);

    static bool Load(
        const std::filesystem::path& path,
        LoadedGameObjectArchive& output,
        bool applySourcePrefabPath);

private:
    class Implementation;
};

#pragma once

#include <string>
#include <utility>

// プロジェクト相対のPrefabパスを保持する。実体の解決はロード側で行う。
class PrefabReference final
{
private:
    std::string m_path;

public:
    PrefabReference() = default;
    explicit PrefabReference(std::string path)
        : m_path(std::move(path))
    {
    }

    const std::string& GetPath() const
    {
        return m_path;
    }

    void SetPath(std::string path)
    {
        m_path = std::move(path);
    }

    void Clear()
    {
        m_path.clear();
    }

    explicit operator bool() const
    {
        return !m_path.empty();
    }
};

#pragma once

#include <filesystem>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <imgui.h>

#include "objlib/object.hpp"
#include "fsystem.hpp"

namespace Toolbox::UI {

    enum class PathType {
        FILE,
        FOLDER,
        ARCHIVE,
    };

    struct PathInfo {
        ImGuiID m_id;

        PathType m_type;
        std::filesystem::path m_name;
        std::vector<char> m_data;

        PathInfo *m_parent;
        PathInfo *m_parent_archive;
        std::vector<std::shared_ptr<PathInfo>> m_children;

        bool operator==(const PathInfo &other) { return m_id == other.m_id; }
    };

    struct FileTypeInfo {
        using expected_t = std::expected<void, Toolbox::FS::FSError>;

        using create_t = std::function<expected_t(const std::filesystem::path &)>;
        using open_t = std::function<expected_t(const std::filesystem::path &)>;

        expected_t open(const std::filesystem::path &path) { return m_open_fn(path); }
        expected_t create(const std::filesystem::path &path) { return m_create_fn(path); }

    private:
        create_t m_create_fn;
        open_t m_open_fn;
    };

    class fs_iterator {
    public:
        using internal_t = std::vector<std::shared_ptr<PathInfo>>::iterator;
        using iterator_category = internal_t::iterator_category;
        using value_type        = internal_t::value_type;
        using difference_type   = internal_t::difference_type;
        using pointer           = internal_t::pointer;
        using reference         = internal_t::reference;

        fs_iterator() : m_current(nullptr) {}

        fs_iterator(std::shared_ptr<PathInfo> root) : m_current(root) {
            m_iterator = root->m_children.begin();
        }

        reference operator*() { return m_iterator.operator*(); }
        pointer operator->() { return m_iterator.operator->(); }

        // Pre-increment
        fs_iterator &operator++() {
            ++m_iterator;
            return *this;
        }

        // Post-increment
        fs_iterator operator++(int) {
            fs_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const fs_iterator &other) const {
            return m_current == other.m_current && m_iterator == other.m_iterator;
        }

        bool operator!=(const fs_iterator &other) const { return !(*this == other); }

    private:
        std::shared_ptr<PathInfo> m_current;
        std::vector<std::shared_ptr<PathInfo>>::iterator m_iterator;
    };

    class recursive_fs_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = PathInfo;
        using difference_type   = std::ptrdiff_t;
        using pointer           = PathInfo *;
        using reference         = PathInfo &;

        recursive_fs_iterator() : m_current(nullptr) {}

        recursive_fs_iterator(std::shared_ptr<PathInfo> root) : m_current(root) {
            if (m_current) {
                m_stack.push(m_current);
            }
        }

        reference operator*() { return *m_stack.top(); }
        pointer operator->() { return m_stack.top().get(); }

        // Pre-increment
        recursive_fs_iterator &operator++() {
            if (m_stack.empty()) {
                m_current = nullptr;
                return *this;
            }

            auto top = m_stack.top();
            m_stack.pop();

            for (auto it = top->m_children.rbegin(); it != top->m_children.rend(); ++it) {
                m_stack.push(*it);
            }

            if (m_stack.empty()) {
                m_current = nullptr;
            } else {
                m_current = m_stack.top();
            }

            return *this;
        }

        // Post-increment
        recursive_fs_iterator operator++(int) {
            recursive_fs_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const recursive_fs_iterator &other) const {
            return m_current == other.m_current;
        }

        bool operator!=(const recursive_fs_iterator &other) const { return !(*this == other); }

    private:
        std::shared_ptr<PathInfo> m_current;
        std::stack<std::shared_ptr<PathInfo>> m_stack;
    };

    class FileSystemManager {
    public:
        using expected_t = FileTypeInfo::expected_t;

        using vector_t = std::vector<std::shared_ptr<PathInfo>>;

        using iterator = vector_t::iterator;
        using const_iterator = vector_t::const_iterator;

        FileSystemManager() = default;
        explicit FileSystemManager(const std::filesystem::path &root_path);
        ~FileSystemManager() = default;

        void registerFileType(std::string_view extension, const FileTypeInfo &info);

        std::filesystem::path cwd() const { return m_cwd; }
        void setCwd(const std::filesystem::path &cwd_) { m_cwd = cwd_; }

        expected_t openPath(const std::filesystem::path &path);

        expected_t createFile(const std::filesystem::path &path);
        expected_t deleteFile(const std::filesystem::path &path);

        expected_t createDirectory(const std::filesystem::path &path);
        expected_t deleteDirectory(const std::filesystem::path &path);

        expected_t renamePath(const std::filesystem::path &src, const std::filesystem::path &dst);
        expected_t copyPath(const std::filesystem::path &src, const std::filesystem::path &dst);

    private:
        std::filesystem::path m_cwd;
    };

}
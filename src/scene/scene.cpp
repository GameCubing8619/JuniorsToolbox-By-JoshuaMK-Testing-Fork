#include "scene/scene.hpp"
#include <fstream>

namespace Toolbox::Scene {

    void ObjectHierarchy::dump(std::ostream &os, size_t indent, size_t indent_size) const {
        std::string indent_str(indent * indent_size, ' ');
        os << indent_str << "ObjectHierarchy (" << m_name << ") {\n";
        m_root->dump(os, indent + 1, indent_size);
        os << indent_str << "}\n";
    }

    std::expected<std::unique_ptr<SceneInstance>, SerialError>
    SceneInstance::FromPath(const std::filesystem::path &root) {
        SceneInstance scene;
        scene.m_root_path = root;

        auto scene_bin   = root / "map/scene.bin";
        auto tables_bin  = root / "map/tables.bin";
        auto rail_bin    = root / "map/scene.ral";
        auto message_bin = root / "map/message.bmg";

        ObjectFactory::create_t map_root_obj;
        ObjectFactory::create_t table_root_obj;

        {
            std::ifstream file(scene_bin, std::ios::in | std::ios::binary);
            Deserializer in(file.rdbuf(), scene_bin.string());

            map_root_obj = ObjectFactory::create(in);
            if (!map_root_obj) {
                return std::unexpected(map_root_obj.error());
            }
        }

        {
            std::ifstream file(tables_bin, std::ios::in | std::ios::binary);
            Deserializer in(file.rdbuf());

            table_root_obj = ObjectFactory::create(in);
            if (!table_root_obj) {
                return std::unexpected(table_root_obj.error());
            }
        }

        std::shared_ptr<Object::GroupSceneObject> map_root_obj_ptr =
            std::static_pointer_cast<GroupSceneObject, ISceneObject>(
                std::move(map_root_obj.value()));
        std::shared_ptr<Object::GroupSceneObject> table_root_obj_ptr =
            std::static_pointer_cast<GroupSceneObject, ISceneObject>(
                std::move(table_root_obj.value()));

        scene.m_map_objects   = ObjectHierarchy("Map", map_root_obj_ptr);
        scene.m_table_objects = ObjectHierarchy("Table", table_root_obj_ptr);

        {
            std::ifstream file(rail_bin, std::ios::in | std::ios::binary);
            Deserializer in(file.rdbuf());

            scene.m_rail_info.deserialize(in);
        }

        {
            std::ifstream file(message_bin, std::ios::in | std::ios::binary);
            Deserializer in(file.rdbuf());

            scene.m_message_data.deserialize(in);
        }

        return std::make_unique<SceneInstance>(std::move(scene));
    }

    SceneInstance::SceneInstance(std::shared_ptr<Object::GroupSceneObject> obj_root)
        : m_map_objects("Map", obj_root), m_table_objects("Table"), m_rail_info(),
          m_message_data() {}

    SceneInstance::SceneInstance(std::shared_ptr<Object::GroupSceneObject> obj_root,
                                 const RailData &rails)
        : m_map_objects("Map", obj_root), m_table_objects("Table"), m_rail_info(rails),
          m_message_data() {}

    SceneInstance::SceneInstance(std::shared_ptr<Object::GroupSceneObject> obj_root,
                                 std::shared_ptr<Object::GroupSceneObject> table_root,
                                 const RailData &rails)
        : m_map_objects("Map", obj_root), m_table_objects("Table", table_root), m_rail_info(rails),
          m_message_data() {}

    std::unique_ptr<SceneInstance> SceneInstance::BasicScene() { return nullptr; }

    std::expected<void, SerialError> SceneInstance::saveToPath(const std::filesystem::path &root) {
        auto scene_bin   = root / "map/scene.bin";
        auto tables_bin  = root / "map/tables.bin";
        auto rail_bin    = root / "map/scene.ral";
        auto message_bin = root / "map/message.bmg";

        {
            std::ofstream file(scene_bin, std::ios::out | std::ios::binary);
            Serializer out(file.rdbuf(), scene_bin.string());

            auto result = m_map_objects.getRoot()->serialize(out);
            if (!result) {
                return std::unexpected(result.error());
            }
        }

        {
            std::ofstream file(tables_bin, std::ios::out | std::ios::binary);
            Serializer out(file.rdbuf(), scene_bin.string());

            auto result = m_table_objects.getRoot()->serialize(out);
            if (!result) {
                return std::unexpected(result.error());
            }
        }

        {
            std::ofstream file(rail_bin, std::ios::out | std::ios::binary);
            Serializer out(file.rdbuf(), scene_bin.string());

            auto result = m_rail_info.serialize(out);
            if (!result) {
                return std::unexpected(result.error());
            }
        }

        {
            std::ofstream file(message_bin, std::ios::out | std::ios::binary);
            Serializer out(file.rdbuf(), scene_bin.string());

            auto result = m_message_data.serialize(out);
            if (!result) {
                return std::unexpected(result.error());
            }
        }

        m_root_path = root;
        return {};
    }

    void SceneInstance::dump(std::ostream &os, size_t indent, size_t indent_size) const {
        std::string indent_str(indent * indent_size, ' ');
        if (!m_root_path) {
            os << indent_str << "SceneInstance (null) {" << std::endl;
        } else {
            os << indent_str << "SceneInstance (" << m_root_path->parent_path().filename() << ") {"
               << std::endl;
        }
        m_map_objects.dump(os, indent + 1, indent_size);
        m_table_objects.dump(os, indent + 1, indent_size);
        m_rail_info.dump(os, indent + 1, indent_size);
        m_message_data.dump(os, indent + 1, indent_size);
        os << indent_str << "}" << std::endl;
    }

    std::unique_ptr<ISmartResource> SceneInstance::clone(bool deep) const {
        if (deep) {
            SceneInstance scene_instance(
                make_deep_clone<Object::GroupSceneObject>(m_map_objects.getRoot()),
                make_deep_clone<Object::GroupSceneObject>(m_table_objects.getRoot()),
                *make_deep_clone<RailData>(m_rail_info));
            return std::make_unique<SceneInstance>(std::move(scene_instance));
        }

        SceneInstance scene_instance(
            make_clone<Object::GroupSceneObject>(m_map_objects.getRoot()),
            make_clone<Object::GroupSceneObject>(m_table_objects.getRoot()),
            *make_clone<RailData>(m_rail_info));
        return std::make_unique<SceneInstance>(std::move(scene_instance));
    }

}  // namespace Toolbox::Scene
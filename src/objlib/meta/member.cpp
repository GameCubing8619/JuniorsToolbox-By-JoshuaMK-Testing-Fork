#include "objlib/meta/member.hpp"
#include "objlib/meta/enum.hpp"
#include "objlib/meta/struct.hpp"
#include "objlib/template.hpp"
#include "objlib/transform.hpp"
#include <algorithm>
#include <optional>

namespace Toolbox::Object {

    QualifiedName MetaMember::qualifiedName() const {
        MetaStruct *parent              = m_parent;
        std::vector<std::string> scopes = {m_name};
        while (parent) {
            scopes.push_back(std::string(parent->name()));
            parent = parent->parent();
        }
        std::reverse(scopes.begin(), scopes.end());
        return QualifiedName(scopes);
    }

    template <>
    std::expected<std::shared_ptr<MetaStruct>, MetaError>
    MetaMember::value<MetaStruct>(size_t index) const {
        if (!validateIndex(index)) {
            return make_meta_error<std::shared_ptr<MetaStruct>>(m_name, index, m_values.size());
        }
        if (!isTypeStruct()) {
            return make_meta_error<std::shared_ptr<MetaStruct>>(
                m_name, "MetaStruct", isTypeValue() ? "MetaValue" : "MetaEnum");
        }
        return std::get<std::shared_ptr<MetaStruct>>(m_values[index]);
    }

    template <>
    std::expected<std::shared_ptr<MetaEnum>, MetaError>
    MetaMember::value<MetaEnum>(size_t index) const {
        if (!validateIndex(index)) {
            return make_meta_error<std::shared_ptr<MetaEnum>>(m_name, index, m_values.size());
        }
        if (!isTypeEnum()) {
            return make_meta_error<std::shared_ptr<MetaEnum>>(
                m_name, "MetaEnum", isTypeValue() ? "MetaValue" : "MetaStruct");
        }
        return std::get<std::shared_ptr<MetaEnum>>(m_values[index]);
    }

    template <>
    std::expected<std::shared_ptr<MetaValue>, MetaError>
    MetaMember::value<MetaValue>(size_t index) const {
        if (!validateIndex(index)) {
            return make_meta_error<std::shared_ptr<MetaValue>>(m_name, index, m_values.size());
        }
        if (!isTypeValue()) {
            return make_meta_error<std::shared_ptr<MetaValue>>(
                m_name, "MetaValue", isTypeStruct() ? "MetaStruct" : "MetaEnum");
        }
        return std::get<std::shared_ptr<MetaValue>>(m_values[index]);
    }

    bool MetaMember::operator==(const MetaMember &other) const {
        return m_name == other.m_name && m_values == other.m_values &&
               m_arraysize == other.m_arraysize && m_parent == other.m_parent;
    }

    void MetaMember::dump(std::ostream &out, size_t indention, size_t indention_width) const {
        indention_width   = std::min(indention_width, size_t(8));
        auto self_indent  = std::string(indention * indention_width, ' ');
        auto value_indent = std::string((indention + 1) * indention_width, ' ');
        if (!isArray()) {
            if (isTypeStruct()) {
                auto _struct = std::get<std::shared_ptr<MetaStruct>>(m_values[0]);
                out << self_indent << _struct->name() << " " << m_name << " ";
                _struct->dump(out, indention, indention_width, true);
                out << ";\n";
            } else if (isTypeEnum()) {
                auto _enum = std::get<std::shared_ptr<MetaEnum>>(m_values[0]);
                out << self_indent << _enum->name() << " " << m_name << " = "
                    << _enum->value().toString() << ";\n";
            } else if (isTypeValue()) {
                auto _value = std::get<std::shared_ptr<MetaValue>>(m_values[0]);
                out << self_indent << meta_type_name(_value->type()) << " " << m_name << " = "
                    << _value->toString() << ";\n";
            }
            return;
        }

        if (isTypeStruct()) {
            out << self_indent << std::get<std::shared_ptr<MetaStruct>>(m_values[0])->name() << " "
                << m_name << " = [\n";
            for (const auto &v : m_values) {
                auto _struct = std::get<std::shared_ptr<MetaStruct>>(v);
                std::cout << value_indent;
                std::get<std::shared_ptr<MetaStruct>>(v)->dump(out, indention + 1, indention_width,
                                                               true);
                std::cout << ",\n";
            }
            out << self_indent << "];\n";
        } else if (isTypeEnum()) {
            out << self_indent << std::get<std::shared_ptr<MetaEnum>>(m_values[0])->name() << " "
                << m_name << " = [\n";
            for (const auto &v : m_values) {
                auto _enum = std::get<std::shared_ptr<MetaEnum>>(v);
                out << _enum->value().toString() << ",\n";
            }
            out << self_indent << "];\n";
        } else if (isTypeValue()) {
            out << self_indent
                << meta_type_name(std::get<std::shared_ptr<MetaEnum>>(m_values[0])->type())
                << m_name << " = [\n";
            for (const auto &v : m_values) {
                auto _value = std::get<std::shared_ptr<MetaValue>>(v);
                out << value_indent << meta_type_name(_value->type()) << m_name << " = "
                    << std::get<std::shared_ptr<MetaValue>>(v)->toString() << ",\n";
            }
            out << self_indent << "];\n";
        }
    }

    std::expected<void, SerialError> MetaMember::serialize(Serializer &out) const {
        for (u32 i = 0; i < arraysize(); ++i) {
            if (isTypeStruct()) {
                auto _struct = std::get<std::shared_ptr<MetaStruct>>(m_values[i]);
                auto result  = _struct->serialize(out);
                if (!result) {
                    return std::unexpected(result.error());
                }
            } else if (isTypeEnum()) {
                auto _enum = std::get<std::shared_ptr<MetaEnum>>(m_values[i]);
                auto result = _enum->serialize(out);
                if (!result) {
                    return std::unexpected(result.error());
                }
            } else if (isTypeValue()) {
                auto _value = std::get<std::shared_ptr<MetaValue>>(m_values[i]);
                auto result = _value->serialize(out);
                if (!result) {
                    return std::unexpected(result.error());
                }
            }
        }
        return {};
    }

    std::expected<void, SerialError> MetaMember::deserialize(Deserializer &in) {
        syncArray();
        for (u32 i = 0; i < arraysize(); ++i) {
            if (isTypeStruct()) {
                auto _struct = std::get<std::shared_ptr<MetaStruct>>(m_values[i]);
                _struct->deserialize(in);
                auto result = _struct->deserialize(in);
                if (!result) {
                    return std::unexpected(result.error());
                }
            } else if (isTypeEnum()) {
                auto _enum = std::get<std::shared_ptr<MetaEnum>>(m_values[i]);
                _enum->deserialize(in);
                auto result = _enum->deserialize(in);
                if (!result) {
                    return std::unexpected(result.error());
                }
            } else if (isTypeValue()) {
                auto _value = std::get<std::shared_ptr<MetaValue>>(m_values[i]);
                auto result = _value->deserialize(in);
                if (!result) {
                    return std::unexpected(result.error());
                }
            }
        }
        return {};
    }

    std::unique_ptr<IClonable> MetaMember::clone(bool deep) const {
        struct protected_ctor_handler : public MetaMember {};

        std::unique_ptr<MetaMember> member = std::make_unique<protected_ctor_handler>();
        member->m_name                     = m_name;
        member->m_parent                   = m_parent;

        if (deep) {
            for (auto &value : m_values) {
                if (isTypeStruct()) {
                    member->m_values.push_back(std::reinterpret_pointer_cast<MetaStruct, IClonable>(
                        std::get<std::shared_ptr<MetaStruct>>(value)->clone(true)));
                } else {
                    auto _copy =
                        std::make_shared<MetaValue>(*std::get<std::shared_ptr<MetaValue>>(value));
                    member->m_values.push_back(_copy);
                }
            }
        } else {
            for (auto &value : m_values) {
                if (isTypeStruct()) {
                    auto _copy =
                        std::make_shared<MetaStruct>(*std::get<std::shared_ptr<MetaStruct>>(value));
                    member->m_values.push_back(_copy);
                } else {
                    auto _copy =
                        std::make_shared<MetaValue>(*std::get<std::shared_ptr<MetaValue>>(value));
                    member->m_values.push_back(_copy);
                }
            }
        }

        return member;
    }

}  // namespace Toolbox::Object
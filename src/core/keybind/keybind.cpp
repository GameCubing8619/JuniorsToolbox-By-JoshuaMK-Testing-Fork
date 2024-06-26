#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "core/input/input.hpp"
#include "core/keybind/keybind.hpp"

namespace Toolbox {
    KeyBind::KeyBind(std::initializer_list<Input::KeyCode> keys) : m_key_combo(keys) {}

    KeyBind::KeyBind(const Input::KeyCodes &keys) : m_key_combo(keys) {}

    KeyBind KeyBind::FromString(const std::string &bind_str) {
        KeyBind result;

        size_t key_begin_index = 0;
        while (true) {
            size_t next_delimiter_index = bind_str.find('+', key_begin_index);
            std::string key_name =
                bind_str.substr(key_begin_index, next_delimiter_index - key_begin_index);
            result.m_key_combo.push_back(KeyNameToEnum(key_name));

            // Last character is + or we reached end of string
            if (next_delimiter_index >= std::string::npos - 1) {
                break;
            }

            key_begin_index = next_delimiter_index + 1;
        }

        return result;
    }

    bool KeyBind::isInputMatching() const {
        return std::all_of(m_key_combo.begin(), m_key_combo.end(),
                           [](Input::KeyCode keybind) { return Input::GetKey(keybind); });
    }

    std::string KeyBind::toString() const {
        std::string keybind_name = "";
        for (size_t j = 0; j < m_key_combo.size(); ++j) {
            Input::KeyCode key = m_key_combo.at(j);
            keybind_name += KeyNameFromEnum(key);
            if (j < m_key_combo.size() - 1) {
                keybind_name += "+";
            }
        }
        return keybind_name;
    }

    bool KeyBind::scanNextInputKey() {
        // Check if any of the keys are still being held
        bool any_keys_held = std::any_of(m_key_combo.begin(), m_key_combo.end(),
                                         [](Input::KeyCode key) { return Input::GetKey(key); });
        if (m_key_combo.size() > 0 && !any_keys_held) {
            return true;
        }
        // Check if any new keys have been pressed
        for (auto key : Input::GetKeyCodes()) {
            bool is_already_held =
                std::any_of(m_key_combo.begin(), m_key_combo.end(),
                            [&](Input::KeyCode our_key) { return our_key == key; });
            if (!is_already_held && Input::GetKey(key)) {
                m_key_combo.push_back(key);
            }
        }
        // Force a max length of 3 keys
        return m_key_combo.size() >= 3;
    }

    bool KeyBind::operator==(const KeyBind &other) const {
        return m_key_combo == other.m_key_combo;
    }

    bool KeyBind::operator!=(const KeyBind &other) const {
        return m_key_combo != other.m_key_combo;
    }

}  // namespace Toolbox
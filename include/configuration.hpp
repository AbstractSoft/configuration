#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <string_view>

namespace configuration {

inline constexpr char const* DEFAULT_CONFIG_PATH = "./config.json";

class Configuration {
public:
    explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
        : m_config_path(config_path) {
        load();
    }

    template <typename T>
    T get(std::string_view key, T default_value = {}) const {
        nlohmann::json const* json = find_key(key);
        if (json) {
            try {
                return json->get<T>();
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    bool has(std::string_view key) const {
        return find_key(key) != nullptr;
    }

private:
    nlohmann::json const* find_key(std::string_view key) const {
        nlohmann::json const* current = &m_json;
        std::size_t pos = 0;

        while (pos < key.size()) {
            auto dot = key.find('.', pos);
            auto part = key.substr(pos, dot == std::string_view::npos ? key.size() - pos : dot - pos);

            if (!current->is_object()) {
                return nullptr;
            }

            auto it = current->find(std::string(part));
            if (it == current->end()) {
                return nullptr;
            }

            current = &*it;

            if (dot == std::string_view::npos) {
                break;
            }
            pos = dot + 1;
        }

        return current;
    }

    void load() {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + m_config_path);
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (nlohmann::json::parse_error const& e) {
            throw std::runtime_error("JSON parse error in '" + m_config_path + "': " + e.what());
        }
    }

    std::string m_config_path;
    nlohmann::json m_json;
};

} // namespace configuration

#endif // CONFIGURATION_HPP

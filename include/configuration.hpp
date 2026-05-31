#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace configuration {

class Section {
protected:
    template<typename T>
    static T read(nlohmann::json const& json, std::string_view key, T default_value) {
        auto it = json.find(key);
        if (it != json.end()) {
            if constexpr (std::is_same_v<T, char const*>) {
                return it->get_ref<std::string const&>().c_str();
            } else {
                return it->get<T>();
            }
        }
        return default_value;
    }
};

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = "./config.json")
        : m_config_path(config_path) {}

    template<typename S>
    const S& get() const {
        if (!m_loaded) {
            load();
        }
        return std::get<S>(m_sections);
    }

private:
    std::string m_config_path;
    mutable nlohmann::json m_json;
    mutable std::tuple<Sections...> m_sections;
    mutable bool m_loaded = false;

    void load() const {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + m_config_path);
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error in '" + m_config_path + "': " + e.what());
        }
        deserialize_all();
        m_loaded = true;
    }

    template<typename S>
    void deserialize_section(nlohmann::json const& json) const {
        auto& section_ref = std::get<S>(m_sections);
        auto it = json.find(S::section_name());
        if (it == json.end()) {
            throw std::runtime_error("Missing required section '" + std::string(S::section_name()) + "' in configuration.json");
        }
        section_ref.load(*it);
    }

    void deserialize_all() const {
        int dummy[] = {0, (deserialize_section<Sections>(m_json), 0)...};
        (void)dummy;
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP

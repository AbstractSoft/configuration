#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>

namespace configuration {

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path)
        : m_config_path(std::string(config_path)) {
        load_from_file();
    }

    static Configuration from_json(nlohmann::json json_data) {
        Configuration config;
        config.m_json = std::move(json_data);
        config.deserialize_all();
        return config;
    }

    template<typename S>
    const S& get() const {
        return std::get<S>(m_sections);
    }

private:
    Configuration() = default;

    std::string m_config_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;

    void load_from_file() {
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
    }

    template<typename S>
    void deserialize_section(nlohmann::json& json, std::tuple<Sections...>& sections) {
        auto& section_ref = std::get<S>(sections);
        auto it = json.find(S::section_name());
        if (it == json.end()) {
            throw std::runtime_error("Missing required section '" + std::string(S::section_name()) + "' in configuration.json");
        }
        S::from_json(section_ref, *it);
    }

    void deserialize_all() {
        int dummy[] = {0, (deserialize_section<Sections>(m_json, m_sections), 0)...};
        (void)dummy;
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP

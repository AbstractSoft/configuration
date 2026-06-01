#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <any>
#include <map>
#include <string>
#include <string_view>
#include <typeindex>

namespace configuration {

inline constexpr char const* DEFAULT_CONFIG_PATH = "./config.json";

class Configuration {
public:
    explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
        : m_config_path(config_path) {
        load();
    }

    template <typename SectionT>
    SectionT get() const {
        auto key = std::type_index{typeid(SectionT)};
        auto it = m_sections.find(key);
        if (it != m_sections.end()) {
            return *std::any_cast<SectionT>(&it->second);
        }

        SectionT section;
        auto json_it = m_json.find(SectionT::section_name());
        if (json_it != m_json.end()) {
            for_each_field(*json_it, section);
        }
        m_sections.emplace(key, std::move(section));
        return *std::any_cast<SectionT>(&m_sections.at(key));
    }

private:
    void load() {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + std::string(m_config_path));
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (nlohmann::json::parse_error const& e) {
            throw std::runtime_error("JSON parse error in '" + m_config_path + "': " + e.what());
        }
    }

    template <typename SectionT>
    static void for_each_field(nlohmann::json const& json, SectionT& section) {
        SectionT::for_each_field(section, [&json](auto& field, std::string_view key) {
            using FieldT = std::remove_reference_t<decltype(field)>;
            if (json.contains(key)) {
                field = json[key].get<FieldT>();
            }
        });
    }

    std::string m_config_path;
    nlohmann::json m_json;
    mutable std::map<std::type_index, std::any> m_sections;
};

} // namespace configuration

#endif // CONFIGURATION_HPP

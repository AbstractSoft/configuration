#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tuple>

namespace configuration {

inline constexpr char const* DEFAULT_CONFIG_PATH = "./config.json";

template <typename Derived>
class Section {
public:
    void initialize(nlohmann::json const& json) {
        m_json = &json;
        static_cast<Derived*>(this)->load();
    }

protected:
    nlohmann::json const* m_json = nullptr;

    template <typename T>
    [[nodiscard]] T read(std::string_view key, T default_value) const {
        if (m_json && m_json->contains(key)) {
            return m_json->value(key, default_value);
        }
        return default_value;
    }
};

template <typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
        : m_config_path(config_path) {
        load();
    }

    template <typename S>
    [[nodiscard]] S const& get() const {
        return std::get<S>(m_sections);
    }

private:
    std::string m_config_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;

    void load() {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + m_config_path);
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (nlohmann::json::parse_error const& e) {
            throw std::runtime_error(
                "JSON parse error in '" + m_config_path + "': " + e.what());
        }
        construct_sections();
    }

    template <typename S>
    void construct_section() {
        auto it = m_json.find(S::section_name());
        if (it == m_json.end()) {
            throw std::runtime_error(
                "Missing required section '" + std::string(S::section_name()) +
                "' in " + m_config_path);
        }
        std::get<S>(m_sections).initialize(*it);
    }

    void construct_sections() {
        (construct_section<Sections>(), ...);
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP

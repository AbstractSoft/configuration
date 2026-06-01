#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace configuration {

inline constexpr char const* DEFAULT_CONFIG_PATH = "./config.json";

class Section {
public:
    explicit Section(std::string_view name) : m_name(name) {}
    virtual ~Section() = default;

    std::string_view get_name() const { return m_name; }

protected:
    nlohmann::json const* m_json = nullptr;

    template<typename T>
    T read(std::string_view key, T default_value) const {
        if (m_json && m_json->contains(key)) {
            return m_json->value(key, default_value);
        }
        return default_value;
    }

    void set_json(nlohmann::json const* json) { m_json = json; }

private:
    std::string m_name;
};

template<typename S>
const S& get_section(std::vector<std::unique_ptr<Section>> const& sections) {
    for (auto const& sec : sections) {
        if (sec->get_name() == S::section_name()) {
            return static_cast<S const&>(*sec);
        }
    }
    throw std::runtime_error("Section '" + std::string(S::section_name()) + "' not found");
}

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
        : m_config_path(config_path) {
        load();
    }

    template<typename S>
    const S& get() const {
        return get_section<S>(m_sections);
    }

private:
    std::string m_config_path;
    nlohmann::json m_json;
    std::vector<std::unique_ptr<Section>> m_sections;

    void load() {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + m_config_path);
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error in '" + m_config_path + "': " + e.what());
        }
        construct_sections();
    }

    template<typename S>
    void construct_section() {
        auto it = m_json.find(S::section_name());
        if (it == m_json.end()) {
            throw std::runtime_error("Missing required section '" + std::string(S::section_name()) + "' in " + m_config_path);
        }
        m_sections.push_back(std::make_unique<S>(m_json[S::section_name()]));
    }

    template<size_t... Is>
    void construct_sections_impl(std::index_sequence<Is...>) {
        using unused = int[];
        (void)unused{0, (construct_section<typename std::tuple_element<Is, std::tuple<Sections...>>::type>(), 0)...};
    }

    void construct_sections() {
        construct_sections_impl(std::index_sequence_for<Sections...>{});
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tuple>

namespace configuration {

inline constexpr char const* DEFAULT_CONFIG_PATH = "./config.json";

class Section {
public:
    explicit Section(std::string_view name) : m_name(name) {}

    void set_json(nlohmann::json const* json) { m_json = json; }
    std::string_view get_name() const { return m_name; }

protected:
    mutable nlohmann::json const* m_json = nullptr;

    template<typename T>
    T read(std::string_view key, T default_value) const {
        if (m_json && m_json->contains(key)) {
            return m_json->value(key, default_value);
        }
        return default_value;
    }

    virtual void populate() = 0;
    virtual ~Section() = default;

private:
    std::string m_name;
};

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
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
        for_each_section([this](auto& section) {
            auto it = m_json.find(section.get_name());
            if (it == m_json.end()) {
                throw std::runtime_error("Missing required section '" + std::string(section.get_name()) + "' in " + m_config_path);
            }
            section.set_json(&m_json[section.get_name()]);
            section.populate();
        });
        m_loaded = true;
    }

    template<typename Callable, size_t... Is>
    void for_each_section(Callable&& fn, std::index_sequence<Is...>) const {
        int dummy[] = {0, (fn(std::get<Is>(m_sections)), 0)...};
        (void)dummy;
    }

    template<typename Callable>
    void for_each_section(Callable&& fn) const {
        for_each_section(std::forward<Callable>(fn), std::index_sequence_for<Sections...>{});
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP

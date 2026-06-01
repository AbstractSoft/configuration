#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace configuration
{
    inline constexpr auto DEFAULT_CONFIG_PATH = "./config.json";

    class Configuration
    {
    public:
        explicit Configuration(std::string_view config_path = DEFAULT_CONFIG_PATH)
            : m_config_path(config_path)
        {
            load();
        }

        // Returns the value at `key` (dot-separated for nesting), or `default_value`
        // if the key is absent. Throws std::runtime_error if the key exists but
        // cannot be converted to T.
        //
        // For struct types: T must either inherit Reflectable<T> or be supported
        // by nlohmann directly (e.g. via NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE).
        template <typename T>
        [[nodiscard]] T get(const std::string_view key, T default_value = {}) const
        {
            nlohmann::json const* node = find_key(key);
            if (!node)
            {
                return default_value;
            }
            try
            {
                return node->get<T>();
            }
            catch (nlohmann::json::exception const& e)
            {
                throw std::runtime_error(
                    "Configuration: cannot convert key '" + std::string(key) +
                    "' to the requested type: " + e.what());
            }
        }

        // Returns the value at `key` as std::optional<T>, or std::nullopt if absent.
        // Throws std::runtime_error if the key exists but cannot be converted to T.
        template <typename T>
        [[nodiscard]] auto try_get(std::string_view key) const -> std::optional<T>
        {
            nlohmann::json const* node = find_key(key);
            if (!node)
            {
                return std::nullopt;
            }
            try
            {
                return node->get<T>();
            }
            catch (nlohmann::json::exception const& e)
            {
                throw std::runtime_error(
                    "Configuration: cannot convert key '" + std::string(key) +
                    "' to the requested type: " + e.what());
            }
        }

        // Returns true if the key exists (leaf or intermediate node).
        [[nodiscard]] bool has(std::string_view key) const
        {
            return find_key(key) != nullptr;
        }

    private:
        // Traverses dot-separated key segments. Returns nullptr if any segment
        // is missing, or if a non-object node is encountered mid-path.
        // An empty key returns nullptr.
        [[nodiscard]] nlohmann::json const* find_key(const std::string_view key) const
        {
            if (key.empty())
            {
                return nullptr;
            }

            nlohmann::json const* current = &m_json;
            std::size_t pos = 0;

            while (pos < key.size())
            {
                auto dot = key.find('.', pos);
                auto part = (dot == std::string_view::npos)
                                ? key.substr(pos)
                                : key.substr(pos, dot - pos);

                if (!current->is_object())
                {
                    return nullptr;
                }

                // nlohmann::find does not accept string_view directly;
                // The string construction here is intentional.
                auto it = current->find(std::string(part));
                if (it == current->end())
                {
                    return nullptr;
                }

                current = &*it;
                pos = (dot == std::string_view::npos) ? key.size() : dot + 1;
            }

            return current;
        }

        void load()
        {
            std::ifstream file(m_config_path);
            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open configuration file: " + m_config_path);
            }
            try
            {
                m_json = nlohmann::json::parse(file);
            }
            catch (nlohmann::json::parse_error const& e)
            {
                throw std::runtime_error(
                    "JSON parse error in '" + m_config_path + "': " + e.what());
            }
        }

        std::string m_config_path;
        nlohmann::json m_json;
    };
} // namespace configuration

#endif // CONFIGURATION_HPP

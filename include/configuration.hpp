/*
 * Configuration library
 *
 * Copyright (C) 2026 Eduard Ghergu, PhD <eduard.ghergu@professional-programmer.com>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
        explicit Configuration(const std::string_view config_path = DEFAULT_CONFIG_PATH)
            : config_path{config_path}
        {
            load();
        }

        // Returns the value at `key` (dot-separated for nesting), or `default_value.`
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
            return convert<T>(key, *node);
        }

        // Returns the value at `key` as std::optional<T>, or std::nullopt if absent.
        // Throws std::runtime_error if the key exists but cannot be converted to T.
        template <typename T>
        [[nodiscard]] auto try_get(const std::string_view key) const -> std::optional<T>
        {
            nlohmann::json const* node = find_key(key);
            if (!node)
            {
                return std::nullopt;
            }
            return convert<T>(key, *node);
        }

        // Returns true if the key exists (leaf or intermediate node).
        [[nodiscard]] bool has(const std::string_view key) const
        {
            return find_key(key) != nullptr;
        }

    private:
        template <typename T>
        [[nodiscard]] static T convert(const std::string_view key, nlohmann::json const& node)
        {
            try
            {
                return node.get<T>();
            }
            catch (nlohmann::json::exception const& e)
            {
                throw std::runtime_error{
                    "Configuration: cannot convert key '" + std::string(key) +
                    "' to the requested type: " + e.what()
                };
            }
        }

        // Traverses dot-separated key segments. Returns nullptr if any segment
        // is missing, or if a non-object node is encountered mid-path.
        // An empty key returns nullptr.
        [[nodiscard]] nlohmann::json const* find_key(const std::string_view key) const
        {
            if (key.empty())
            {
                return nullptr;
            }

            nlohmann::json const* current = &json;
            std::size_t pos = 0;

            while (pos < key.size())
            {
                const auto dot = key.find('.', pos);
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
            std::ifstream file{config_path};
            if (!file.is_open())
            {
                throw std::runtime_error{"Cannot open configuration file: " + config_path};
            }
            try
            {
                json = nlohmann::json::parse(file);
            }
            catch (nlohmann::json::parse_error const& e)
            {
                throw std::runtime_error{
                    "JSON parse error in '" + config_path + "': " + e.what()
                };
            }
        }

        std::string config_path;
        nlohmann::json json;
    };
} // namespace configuration

#endif // CONFIGURATION_HPP

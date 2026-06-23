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

#include "configuration.hpp"
#include "field_reflection.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Section structs — fields() is the only obligation; serialization is inherited
// ---------------------------------------------------------------------------

struct Cache : Reflectable<Cache>
{
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    [[maybe_unused]] static constexpr auto fields()
    {
        return std::tuple{
            Field{"enabled", &Cache::enabled},
            Field{"path", &Cache::path},
            Field{"default_ttl_seconds", &Cache::default_ttl_seconds}
        };
    }
};

struct Server : Reflectable<Server>
{
    std::string host = "127.0.0.1";
    int port = 8080;

    [[maybe_unused]] static constexpr auto fields()
    {
        return std::tuple{
            Field{"host", &Server::host},
            Field{"port", &Server::port}
        };
    }
};

struct Account : Reflectable<Account>
{
    std::string name;
    std::string imap_server;
    int port = 0;
    std::string username;
    std::string password;
    std::string save_folder;

    [[maybe_unused]] static constexpr auto fields()
    {
        return std::tuple{
            Field{"name", &Account::name},
            Field{"imap_server", &Account::imap_server},
            Field{"port", &Account::port},
            Field{"username", &Account::username},
            Field{"password", &Account::password},
            Field{"save_folder", &Account::save_folder}
        };
    }
};

struct Application : Reflectable<Application>
{
    Server server;
    Cache cache;

    [[maybe_unused]] static constexpr auto fields()
    {
        return std::tuple{
            Field{"server", &Application::server},
            Field{"cache", &Application::cache}
        };
    }
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class configuration_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        static std::atomic<int> counter{0};
        auto unique = std::to_string(counter.fetch_add(1));
        test_dir = fs::temp_directory_path() / ("config_test_" + unique);
        fs::create_directories(test_dir);
        config_path = test_dir / "configuration.json";
    }

    void TearDown() override
    {
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
    }

    void write_json(nlohmann::json const& j) const
    {
        std::ofstream file{config_path};
        file << j.dump(4);
    }

    fs::path test_dir;
    fs::path config_path;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(configuration_test, GetPrimitiveTypes)
{
    write_json({
        {"enabled", false}, {"port", 8080}, {"timeout", 3.14},
        {"path", "/tmp/test.db"}, {"count", 42}
    });

    configuration::Configuration config{config_path.string()};

    EXPECT_FALSE(config.get<bool>("enabled"));
    EXPECT_EQ(config.get<int>("port"), 8080);
    EXPECT_DOUBLE_EQ(config.get<double>("timeout"), 3.14);
    EXPECT_EQ(config.get<std::string>("path"), "/tmp/test.db");
    EXPECT_EQ(config.get<int>("count"), 42);
}

TEST_F(configuration_test, GetWithDefault)
{
    write_json({{"enabled", true}});

    configuration::Configuration config{config_path.string()};

    EXPECT_TRUE(config.get<bool>("enabled", false));
    EXPECT_TRUE(config.get<bool>("missing", true));
    EXPECT_EQ(config.get<int>("port", 9090), 9090);
    EXPECT_EQ(config.get<std::string>("path", "./default.db"), "./default.db");
}

TEST_F(configuration_test, GetNestedKeys)
{
    nlohmann::json j;
    j["cache"]["enabled"] = false;
    j["cache"]["ttl"] = 300;
    j["server"]["port"] = 9090;
    write_json(j);

    configuration::Configuration config{config_path.string()};

    EXPECT_FALSE(config.get<bool>("cache.enabled", true));
    EXPECT_EQ(config.get<int>("cache.ttl", 60), 300);
    EXPECT_EQ(config.get<int>("server.port", 8080), 9090);
}

TEST_F(configuration_test, GetDeepNestedKeys)
{
    nlohmann::json j;
    j["a"]["b"]["c"]["d"] = 42;
    j["a"]["b"]["c"]["e"] = "deep";
    write_json(j);

    configuration::Configuration config{config_path.string()};

    EXPECT_EQ(config.get<int>("a.b.c.d", 0), 42);
    EXPECT_EQ(config.get<std::string>("a.b.c.e", "default"), "deep");
    EXPECT_EQ(config.get<int>("a.b.c.missing", -1), -1);
}

TEST_F(configuration_test, HasMethod)
{
    nlohmann::json j;
    j["enabled"] = true;
    j["cache"]["enabled"] = false;
    write_json(j);

    configuration::Configuration config{config_path.string()};

    EXPECT_TRUE(config.has("enabled"));
    EXPECT_TRUE(config.has("cache")); // intermediate node
    EXPECT_TRUE(config.has("cache.enabled")); // nested leaf
    EXPECT_FALSE(config.has("missing"));
    EXPECT_FALSE(config.has("cache.missing"));
    EXPECT_FALSE(config.has("")); // empty key guard
}

TEST_F(configuration_test, WrongTypeThrows)
{
    write_json({{"port", "not_a_number"}});

    configuration::Configuration config{config_path.string()};

    EXPECT_THROW((void)config.get<int>("port"), std::runtime_error);
}

TEST_F(configuration_test, GetTypedObject)
{
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/cache.db"}, {"default_ttl_seconds", 600}};
    write_json(j);

    configuration::Configuration config{config_path.string()};
    const auto cache = config.get<Cache>("cache");

    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);
}

// Key advantage over NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE: absent fields keep
// their C++ default values instead of throwing.
TEST_F(configuration_test, GetTypedObjectPartialJson)
{
    nlohmann::json j;
    j["cache"] = {{"enabled", false}}; // path and default_ttl_seconds absent
    write_json(j);

    configuration::Configuration config{config_path.string()};
    const auto cache = config.get<Cache>("cache");

    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db"); // C++ default preserved
    EXPECT_EQ(cache.default_ttl_seconds, 300); // C++ default preserved
}

TEST_F(configuration_test, GetTypedObjectWithDefault)
{
    write_json(nlohmann::json::object());

    configuration::Configuration config{config_path.string()};
    const auto cache = config.get<Cache>("missing", Cache{});

    EXPECT_TRUE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 300);
}

TEST_F(configuration_test, TryGetReturnsNulloptForMissingKey)
{
    write_json({{"port", 8080}, {"path", "/tmp/test"}});

    configuration::Configuration config{config_path.string()};

    EXPECT_FALSE(config.try_get<int>("missing"));
    EXPECT_EQ(config.try_get<int>("port").value(), 8080);
    EXPECT_FALSE(config.try_get<std::string>("nonexistent"));
    EXPECT_EQ(config.try_get<std::string>("path").value(), "/tmp/test");
}

TEST_F(configuration_test, TryGetThrowsOnConversionError)
{
    write_json({{"port", "not_a_number"}});

    configuration::Configuration config{config_path.string()};
    EXPECT_THROW((void)config.try_get<int>("port"), std::runtime_error);
}

TEST_F(configuration_test, GetNestedReflectableTypes)
{
    nlohmann::json j;
    j["app"]["server"]["host"] = "0.0.0.0";
    j["app"]["server"]["port"] = 3000;
    j["app"]["cache"]["enabled"] = false;
    j["app"]["cache"]["path"] = "/var/cache/app";
    write_json(j);

    configuration::Configuration config{config_path.string()};
    const auto app = config.get<Application>("app");

    EXPECT_EQ(app.server.host, "0.0.0.0");
    EXPECT_EQ(app.server.port, 3000);
    EXPECT_FALSE(app.cache.enabled);
    EXPECT_EQ(app.cache.path, "/var/cache/app");
    EXPECT_EQ(app.cache.default_ttl_seconds, 300); // C++ default preserved
}

TEST_F(configuration_test, GetTypedArray)
{
    nlohmann::json j;
    j["accounts"] = {
        {
            {"name", "gmail.com"}, {"imap_server", "imap.gmail.com"}, {"port", 993},
            {"username", "professional@gmail.com"}, {"password", "pw1"}, {"save_folder", "work"}
        },
        {
            {"name", "gmail.com"}, {"imap_server", "imap.gmail.com"}, {"port", 993},
            {"username", "personal@gmail.com"}, {"password", "pw2"}, {"save_folder", "home"}
        }
    };
    write_json(j);

    configuration::Configuration config{config_path.string()};
    auto accounts = config.get<std::vector<Account>>("accounts");

    ASSERT_EQ(accounts.size(), 2u);
    EXPECT_EQ(accounts[0].username, "professional@gmail.com");
    EXPECT_EQ(accounts[0].save_folder, "work");
    EXPECT_EQ(accounts[0].port, 993);
    EXPECT_EQ(accounts[1].username, "personal@gmail.com");
    EXPECT_EQ(accounts[1].save_folder, "home");
}

TEST_F(configuration_test, MalformedJsonThrows)
{
    std::ofstream{config_path} << "{ invalid json }";
    EXPECT_THROW(configuration::Configuration{config_path.string()}, std::runtime_error);
}

TEST_F(configuration_test, MissingFileThrows)
{
    EXPECT_THROW(
        configuration::Configuration{(test_dir / "nonexistent.json").string()},
        std::runtime_error);
}

TEST_F(configuration_test, DefaultConstructorCreatesEmptyJson)
{
    configuration::Configuration config;
    EXPECT_EQ(config.get<int>("missing", 42), 42);
    EXPECT_FALSE(config.has("anything"));
}

TEST_F(configuration_test, SetPrimitiveValue)
{
    configuration::Configuration config;
    config.set("port", 8080);
    config.set("enabled", true);
    config.set("name", std::string{"test"});

    EXPECT_EQ(config.get<int>("port"), 8080);
    EXPECT_TRUE(config.get<bool>("enabled"));
    EXPECT_EQ(config.get<std::string>("name"), "test");
}

TEST_F(configuration_test, SetNestedValue)
{
    configuration::Configuration config;
    config.set("server.host", "0.0.0.0");
    config.set("server.port", 3000);
    config.set("cache.enabled", false);

    EXPECT_EQ(config.get<std::string>("server.host"), "0.0.0.0");
    EXPECT_EQ(config.get<int>("server.port"), 3000);
    EXPECT_FALSE(config.get<bool>("cache.enabled"));
}

TEST_F(configuration_test, SetOverwritesExistingValue)
{
    write_json({{"port", 8080}});
    configuration::Configuration config{config_path.string()};

    config.set("port", 9090);
    EXPECT_EQ(config.get<int>("port"), 9090);
}

TEST_F(configuration_test, SetThrowsOnNonObjectPath)
{
    write_json({{"port", 8080}});
    configuration::Configuration config{config_path.string()};

    EXPECT_THROW(config.set("port.host", "localhost"), std::runtime_error);
}

TEST_F(configuration_test, SaveWritesToFile)
{
    configuration::Configuration config;
    config.set("port", 8080);
    config.set("host", "0.0.0.0");
    config.save_as(config_path.string());

    std::ifstream file{config_path};
    nlohmann::json saved = nlohmann::json::parse(file);
    EXPECT_EQ(saved["port"], 8080);
    EXPECT_EQ(saved["host"], "0.0.0.0");
}

TEST_F(configuration_test, SaveAsWritesToDifferentPath)
{
    configuration::Configuration config;
    config.set("value", 42);

    auto save_path = test_dir / "output.json";
    config.save_as(save_path.string());

    std::ifstream file{save_path};
    nlohmann::json saved = nlohmann::json::parse(file);
    EXPECT_EQ(saved["value"], 42);
}

TEST_F(configuration_test, SaveAsThrowsOnInvalidPath)
{
    configuration::Configuration config;
    config.set("value", 42);

    EXPECT_THROW(config.save_as("/nonexistent/directory/config.json"), std::runtime_error);
}

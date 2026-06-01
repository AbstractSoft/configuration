#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include "configuration.hpp"

namespace fs = std::filesystem;

struct Cache {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;
};

struct Server {
    std::string host = "127.0.0.1";
    int port = 8080;
};

struct Account {
    std::string name;
    std::string imap_server;
    int port;
    std::string username;
    std::string password;
    std::string save_folder;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cache, enabled, path, default_ttl_seconds)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Server, host, port)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Account, name, imap_server, port, username, password, save_folder)

class ConfigurationTest : public ::testing::Test {
protected:
    void SetUp() {
        test_dir = fs::temp_directory_path() / "config_test_XXXXXX";
        fs::create_directories(test_dir);
        config_path = test_dir / "configuration.json";
    }

    void TearDown() {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    fs::path test_dir;
    fs::path config_path;

    void write_json(nlohmann::json const& j) {
        std::ofstream file(config_path);
        file << j.dump(4);
    }
};

TEST_F(ConfigurationTest, GetPrimitiveTypes) {
    nlohmann::json j = {
        {"enabled", false},
        {"port", 8080},
        {"timeout", 3.14},
        {"path", "/tmp/test.db"},
        {"count", 42}
    };
    write_json(j);

    configuration::Configuration config(config_path.string());

    bool enabled = config.get<bool>("enabled");
    int port = config.get<int>("port");
    double timeout = config.get<double>("timeout");
    std::string path = config.get<std::string>("path");
    int count = config.get<int>("count");

    EXPECT_FALSE(enabled);
    EXPECT_EQ(port, 8080);
    EXPECT_DOUBLE_EQ(timeout, 3.14);
    EXPECT_EQ(path, "/tmp/test.db");
    EXPECT_EQ(count, 42);
}

TEST_F(ConfigurationTest, GetWithDefault) {
    nlohmann::json j = {{"enabled", true}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    bool enabled = config.get<bool>("enabled", false);
    bool missing = config.get<bool>("missing", true);
    int port = config.get<int>("port", 9090);
    std::string path = config.get<std::string>("path", "./default.db");

    EXPECT_TRUE(enabled);
    EXPECT_TRUE(missing);
    EXPECT_EQ(port, 9090);
    EXPECT_EQ(path, "./default.db");
}

TEST_F(ConfigurationTest, GetNestedKeys) {
    nlohmann::json j;
    j["cache"]["enabled"] = false;
    j["cache"]["ttl"] = 300;
    j["server"]["port"] = 9090;
    write_json(j);

    configuration::Configuration config(config_path.string());

    bool cache_enabled = config.get<bool>("cache.enabled", true);
    int cache_ttl = config.get<int>("cache.ttl", 60);
    int server_port = config.get<int>("server.port", 8080);

    EXPECT_FALSE(cache_enabled);
    EXPECT_EQ(cache_ttl, 300);
    EXPECT_EQ(server_port, 9090);
}

TEST_F(ConfigurationTest, HasMethod) {
    nlohmann::json j = {{"enabled", true}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    EXPECT_TRUE(config.has("enabled"));
    EXPECT_FALSE(config.has("missing"));
    EXPECT_FALSE(config.has("cache.enabled"));
}

TEST_F(ConfigurationTest, MalformedJsonThrows) {
    std::ofstream file(config_path);
    file << "{ invalid json }";
    file.close();

    EXPECT_THROW(configuration::Configuration config(config_path.string()), std::runtime_error);
}

TEST_F(ConfigurationTest, GetTypedObject) {
    nlohmann::json j;
    j["cache"] = {
        {"enabled", false},
        {"path", "/tmp/cache.db"},
        {"default_ttl_seconds", 600}
    };
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache = config.get<Cache>("cache");

    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);
}

TEST_F(ConfigurationTest, GetTypedObjectWithDefaults) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache = config.get<Cache>("cache");

    EXPECT_TRUE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 300);
}

TEST_F(ConfigurationTest, GetTypedArray) {
    nlohmann::json j;
    j["accounts"] = {
        {
            {"name", "gmail.com"},
            {"imap_server", "imap.gmail.com"},
            {"port", 993},
            {"username", "professional.programmer.com@gmail.com"},
            {"password", "app_password_1"},
            {"save_folder", "gmail_professional"}
        },
        {
            {"name", "gmail.com"},
            {"imap_server", "imap.gmail.com"},
            {"port", 993},
            {"username", "another@gmail.com"},
            {"password", "app_password_2"},
            {"save_folder", "gmail_personal"}
        }
    };
    write_json(j);

    configuration::Configuration config(config_path.string());

    std::vector<Account> accounts = config.get<std::vector<Account>>("accounts");

    ASSERT_EQ(accounts.size(), 2u);
    EXPECT_EQ(accounts[0].username, "professional.programmer.com@gmail.com");
    EXPECT_EQ(accounts[0].save_folder, "gmail_professional");
    EXPECT_EQ(accounts[0].port, 993);
    EXPECT_EQ(accounts[1].username, "another@gmail.com");
    EXPECT_EQ(accounts[1].save_folder, "gmail_personal");
}

TEST_F(ConfigurationTest, GetTypedObjectWithDefault) {
    nlohmann::json j;
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache = config.get<Cache>("missing", Cache{});

    EXPECT_TRUE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 300);
}

TEST_F(ConfigurationTest, GetDeepNestedKeys) {
    nlohmann::json j;
    j["a"]["b"]["c"]["d"] = 42;
    j["a"]["b"]["c"]["e"] = "deep";
    write_json(j);

    configuration::Configuration config(config_path.string());

    int value = config.get<int>("a.b.c.d", 0);
    std::string str = config.get<std::string>("a.b.c.e", "default");
    int missing = config.get<int>("a.b.c.missing", -1);

    EXPECT_EQ(value, 42);
    EXPECT_EQ(str, "deep");
    EXPECT_EQ(missing, -1);
}

TEST_F(ConfigurationTest, MissingFileThrows) {
    fs::path nonexistent = test_dir / "nonexistent.json";
    EXPECT_THROW(configuration::Configuration config(nonexistent.string()), std::runtime_error);
}

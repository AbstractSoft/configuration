#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "configuration.hpp"

namespace fs = std::filesystem;

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

TEST_F(ConfigurationTest, MissingFileThrows) {
    fs::path nonexistent = test_dir / "nonexistent.json";
    EXPECT_THROW(configuration::Configuration config(nonexistent.string()), std::runtime_error);
}

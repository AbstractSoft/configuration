#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "configuration.hpp"

namespace fs = std::filesystem;

struct Cache {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }

    template <typename Fn>
    static void for_each_field(Cache& self, Fn&& fn) {
        fn(self.enabled, "enabled");
        fn(self.path, "path");
        fn(self.default_ttl_seconds, "default_ttl_seconds");
    }
};

struct Server {
    int port = 8080;
    int thread_pool_size = 10;

    static constexpr std::string_view section_name() { return "server"; }

    template <typename Fn>
    static void for_each_field(Server& self, Fn&& fn) {
        fn(self.port, "port");
        fn(self.thread_pool_size, "thread_pool_size");
    }
};

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

TEST_F(ConfigurationTest, LoadsSectionsFromJson) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    j["server"] = {{"port", 9090}, {"thread_pool_size", 4}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache = config.get<Cache>();
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);

    Server server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 4);
}

TEST_F(ConfigurationTest, ReturnsCachedSections) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache1 = config.get<Cache>();
    Cache cache2 = config.get<Cache>();
    EXPECT_EQ(cache1.enabled, cache2.enabled);
    EXPECT_EQ(cache1.path, cache2.path);
    EXPECT_EQ(cache1.default_ttl_seconds, cache2.default_ttl_seconds);
}

TEST_F(ConfigurationTest, MissingSectionReturnsDefaults) {
    nlohmann::json j;
    j["server"] = {{"port", 9090}};
    write_json(j);

    configuration::Configuration config(config_path.string());

    Cache cache = config.get<Cache>();
    EXPECT_TRUE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 300LL);

    Server server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 10);
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

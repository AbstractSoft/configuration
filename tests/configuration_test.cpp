#include <gtest/gtest.h>
#include "configuration.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

// Test section structs
struct Cache {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }
    static void from_json(Cache& self, nlohmann::json const& j) {
        self.enabled = j.value("enabled", true);
        self.path = j.value("path", "./cache.db");
        self.default_ttl_seconds = j.value("default_ttl_seconds", 300LL);
    }
};

struct Server {
    int port = 8080;
    int thread_pool_size = 10;

    static constexpr std::string_view section_name() { return "server"; }
    static void from_json(Server& self, nlohmann::json const& j) {
        self.port = j.value("port", 8080);
        self.thread_pool_size = j.value("thread_pool_size", 10);
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

    configuration::Configuration<Cache, Server> config(config_path.string());

    auto& cache = config.get<Cache>();
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);

    auto& server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 4);
}

TEST_F(ConfigurationTest, LoadsFromMemoryJson) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    j["server"] = {{"port", 9090}, {"thread_pool_size", 4}};

    auto config = configuration::Configuration<Cache, Server>::from_json(std::move(j));

    auto& cache = config.get<Cache>();
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);

    auto& server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 4);
}

TEST_F(ConfigurationTest, MissingSectionThrows) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}};
    write_json(j);

    EXPECT_THROW((configuration::Configuration<Cache, Server>(config_path.string())), std::runtime_error);
}

TEST_F(ConfigurationTest, MalformedJsonThrows) {
    std::ofstream file(config_path);
    file << "{ invalid json }";
    file.close();

    EXPECT_THROW((configuration::Configuration<Cache>(config_path.string())), std::runtime_error);
}

TEST_F(ConfigurationTest, MissingFileThrows) {
    fs::path nonexistent = test_dir / "nonexistent.json";
    EXPECT_THROW((configuration::Configuration<Cache>(nonexistent.string())), std::runtime_error);
}

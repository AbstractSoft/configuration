#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "configuration.hpp"

namespace fs = std::filesystem;

struct Cache : configuration::Section<Cache> {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }

    void load() {
        enabled = read("enabled", true);
        path = read("path", std::string("./cache.db"));
        default_ttl_seconds = read("default_ttl_seconds", 300LL);
    }
};

struct Server : configuration::Section<Server> {
    int port = 8080;
    int thread_pool_size = 10;

    static constexpr std::string_view section_name() { return "server"; }

    void load() {
        port = read("port", 8080);
        thread_pool_size = read("thread_pool_size", 10);
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

#include <gtest/gtest.h>
#include "core/ConfigManager.h"
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>

using namespace Mouse2VR;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a test config file
        testConfigPath = "test_config.json";
        
        // Clean up any existing test file
        if (std::filesystem::exists(testConfigPath)) {
            std::filesystem::remove(testConfigPath);
        }
        
        config = std::make_unique<ConfigManager>(testConfigPath);
    }
    
    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(testConfigPath)) {
            std::filesystem::remove(testConfigPath);
        }
    }
    
    std::string testConfigPath;
    std::unique_ptr<ConfigManager> config;
};

TEST_F(ConfigManagerTest, DefaultValuesAreSet) {
    // Get default config
    AppConfig appConfig = config->GetConfig();
    
    // Check defaults
    EXPECT_EQ(appConfig.sensitivity, 1.0f);
    EXPECT_EQ(appConfig.deadzone, 0.0f);
    EXPECT_FALSE(appConfig.invertX);
    EXPECT_FALSE(appConfig.invertY);
    EXPECT_TRUE(appConfig.lockX);  // Treadmill locks X by default
    EXPECT_FALSE(appConfig.lockY);
    EXPECT_EQ(appConfig.updateIntervalMs, 20);  // 50Hz
}

TEST_F(ConfigManagerTest, SetAndGetConfig) {
    AppConfig newConfig;
    newConfig.sensitivity = 2.5f;
    newConfig.deadzone = 0.1f;
    newConfig.invertY = true;
    newConfig.updateIntervalMs = 10;
    
    config->SetConfig(newConfig);
    AppConfig retrieved = config->GetConfig();
    
    EXPECT_EQ(retrieved.sensitivity, 2.5f);
    EXPECT_EQ(retrieved.deadzone, 0.1f);
    EXPECT_TRUE(retrieved.invertY);
    EXPECT_EQ(retrieved.updateIntervalMs, 10);
}

TEST_F(ConfigManagerTest, SaveAndLoadConfig) {
    // Set custom values
    AppConfig customConfig;
    customConfig.sensitivity = 1.5f;
    customConfig.invertY = true;
    customConfig.maxSpeed = 2.0f;
    customConfig.showDebugInfo = false;
    
    config->SetConfig(customConfig);
    
    // Save
    EXPECT_TRUE(config->Save());
    
    // Create new instance and load
    auto config2 = std::make_unique<ConfigManager>(testConfigPath);
    EXPECT_TRUE(config2->Load());
    
    // Verify values were persisted
    AppConfig loaded = config2->GetConfig();
    EXPECT_EQ(loaded.sensitivity, 1.5f);
    EXPECT_TRUE(loaded.invertY);
    EXPECT_EQ(loaded.maxSpeed, 2.0f);
    EXPECT_FALSE(loaded.showDebugInfo);
}

TEST_F(ConfigManagerTest, LoadNonExistentFileReturnsFalse) {
    // Use a unique filename that definitely won't exist
    std::string nonExistentPath = "test_non_existent_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".json";
    
    // Make sure the file doesn't exist
    std::filesystem::remove(nonExistentPath);
    
    auto config = std::make_unique<ConfigManager>(nonExistentPath);
    EXPECT_FALSE(config->Load());
    
    // Should still have default values
    AppConfig defaults = config->GetConfig();
    EXPECT_EQ(defaults.sensitivity, 1.0f);
    
    // Clean up in case anything was created
    std::filesystem::remove(nonExistentPath);
}

TEST_F(ConfigManagerTest, CreateDefaultConfig) {
    EXPECT_TRUE(config->CreateDefaultConfig());
    
    // File should exist now
    EXPECT_TRUE(std::filesystem::exists(testConfigPath));
    
    // Should be loadable
    auto config2 = std::make_unique<ConfigManager>(testConfigPath);
    EXPECT_TRUE(config2->Load());
}

TEST_F(ConfigManagerTest, ProcessingConfigConversion) {
    AppConfig appConfig;
    appConfig.sensitivity = 2.0f;
    appConfig.deadzone = 0.05f;
    appConfig.invertX = true;
    appConfig.lockY = true;
    
    ProcessingConfig procConfig = appConfig.toProcessingConfig();
    
    EXPECT_EQ(procConfig.sensitivity, 2.0f);
    EXPECT_EQ(procConfig.deadzone, 0.05f);
    EXPECT_TRUE(procConfig.invertX);
    EXPECT_TRUE(procConfig.lockY);
}

TEST_F(ConfigManagerTest, ThreadSafety) {
    // This test just ensures the methods don't crash when called concurrently
    std::vector<std::thread> threads;
    
    // Multiple readers
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this]() {
            for (int j = 0; j < 100; ++j) {
                auto cfg = config->GetConfig();
                (void)cfg; // Use the config
            }
        });
    }
    
    // One writer
    threads.emplace_back([this]() {
        for (int j = 0; j < 100; ++j) {
            AppConfig cfg;
            cfg.sensitivity = static_cast<float>(j) / 100.0f;
            config->SetConfig(cfg);
        }
    });
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
}
#include <gtest/gtest.h>
#include "core/ConfigManager.h"
#include <filesystem>

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
    // Should have default values even without loading
    EXPECT_GT(config->GetSensitivity(), 0.0);
    EXPECT_GT(config->GetUpdateRate(), 0);
    EXPECT_FALSE(config->GetInvertY());
    EXPECT_FALSE(config->GetAutoStart());
}

TEST_F(ConfigManagerTest, SetAndGetSensitivity) {
    config->SetSensitivity(2.5);
    EXPECT_EQ(config->GetSensitivity(), 2.5);
    
    config->SetSensitivity(0.1);
    EXPECT_EQ(config->GetSensitivity(), 0.1);
}

TEST_F(ConfigManagerTest, SetAndGetUpdateRate) {
    config->SetUpdateRate(120);
    EXPECT_EQ(config->GetUpdateRate(), 120);
    
    config->SetUpdateRate(30);
    EXPECT_EQ(config->GetUpdateRate(), 30);
}

TEST_F(ConfigManagerTest, SetAndGetInvertY) {
    config->SetInvertY(true);
    EXPECT_TRUE(config->GetInvertY());
    
    config->SetInvertY(false);
    EXPECT_FALSE(config->GetInvertY());
}

TEST_F(ConfigManagerTest, SetAndGetAutoStart) {
    config->SetAutoStart(true);
    EXPECT_TRUE(config->GetAutoStart());
    
    config->SetAutoStart(false);
    EXPECT_FALSE(config->GetAutoStart());
}

TEST_F(ConfigManagerTest, SaveAndLoadConfig) {
    // Set custom values
    config->SetSensitivity(1.5);
    config->SetUpdateRate(90);
    config->SetInvertY(true);
    config->SetAutoStart(true);
    
    // Save
    EXPECT_TRUE(config->SaveConfig());
    
    // Create new instance and load
    auto config2 = std::make_unique<ConfigManager>(testConfigPath);
    EXPECT_TRUE(config2->LoadConfig());
    
    // Verify values were persisted
    EXPECT_EQ(config2->GetSensitivity(), 1.5);
    EXPECT_EQ(config2->GetUpdateRate(), 90);
    EXPECT_TRUE(config2->GetInvertY());
    EXPECT_TRUE(config2->GetAutoStart());
}

TEST_F(ConfigManagerTest, LoadNonExistentFileReturnsFalse) {
    auto config = std::make_unique<ConfigManager>("non_existent_file.json");
    EXPECT_FALSE(config->LoadConfig());
    
    // Should still have default values
    EXPECT_GT(config->GetSensitivity(), 0.0);
}

TEST_F(ConfigManagerTest, GetDeadzone) {
    // Deadzone should have a reasonable default
    double deadzone = config->GetDeadzone();
    EXPECT_GE(deadzone, 0.0);
    EXPECT_LE(deadzone, 1.0);
}

TEST_F(ConfigManagerTest, ValidationLimits) {
    // Test sensitivity limits
    config->SetSensitivity(-1.0);
    EXPECT_GT(config->GetSensitivity(), 0.0); // Should be clamped to positive
    
    config->SetSensitivity(100.0);
    EXPECT_LE(config->GetSensitivity(), 10.0); // Should be clamped to max
    
    // Test update rate limits  
    config->SetUpdateRate(0);
    EXPECT_GT(config->GetUpdateRate(), 0); // Should be clamped to minimum
    
    config->SetUpdateRate(1000);
    EXPECT_LE(config->GetUpdateRate(), 240); // Should be clamped to max
}
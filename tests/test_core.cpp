#include <gtest/gtest.h>
#include "core/Mouse2VRCore.h"
#include <thread>
#include <chrono>

class Mouse2VRCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        core = std::make_unique<Mouse2VR::Mouse2VRCore>();
    }
    
    void TearDown() override {
        if (core && core->IsRunning()) {
            core->Stop();
        }
    }
    
    std::unique_ptr<Mouse2VR::Mouse2VRCore> core;
};

TEST_F(Mouse2VRCoreTest, InitializeSucceeds) {
    EXPECT_TRUE(core->Initialize());
    EXPECT_TRUE(core->IsInitialized());
}

TEST_F(Mouse2VRCoreTest, DoubleInitializeIsIdempotent) {
    EXPECT_TRUE(core->Initialize());
    EXPECT_TRUE(core->Initialize()); // Should succeed and return true
    EXPECT_TRUE(core->IsInitialized());
}

TEST_F(Mouse2VRCoreTest, StartRequiresInitialization) {
    // Should not be running before initialization
    EXPECT_FALSE(core->IsRunning());
    
    core->Start();
    EXPECT_FALSE(core->IsRunning()); // Should fail to start
    
    // Initialize then start
    EXPECT_TRUE(core->Initialize());
    core->Start();
    EXPECT_TRUE(core->IsRunning());
}

TEST_F(Mouse2VRCoreTest, StopWorks) {
    EXPECT_TRUE(core->Initialize());
    core->Start();
    EXPECT_TRUE(core->IsRunning());
    
    core->Stop();
    EXPECT_FALSE(core->IsRunning());
}

TEST_F(Mouse2VRCoreTest, GetCurrentStateReturnsValidData) {
    EXPECT_TRUE(core->Initialize());
    
    auto state = core->GetCurrentState();
    EXPECT_EQ(state.speed, 0.0);
    EXPECT_EQ(state.stickX, 0.0);
    EXPECT_EQ(state.stickY, 0.0);
    EXPECT_EQ(state.updateRate, 60);
}

TEST_F(Mouse2VRCoreTest, SetGetSensitivity) {
    EXPECT_TRUE(core->Initialize());
    
    core->SetSensitivity(2.5);
    EXPECT_EQ(core->GetSensitivity(), 2.5);
    
    core->SetSensitivity(0.5);
    EXPECT_EQ(core->GetSensitivity(), 0.5);
}

TEST_F(Mouse2VRCoreTest, ProcessingThreadStartsAndStops) {
    EXPECT_TRUE(core->Initialize());
    
    core->Start();
    EXPECT_TRUE(core->IsRunning());
    
    // Let processing thread run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    core->Stop();
    EXPECT_FALSE(core->IsRunning());
    
    // Give thread time to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_F(Mouse2VRCoreTest, ShutdownCleansUp) {
    EXPECT_TRUE(core->Initialize());
    core->Start();
    EXPECT_TRUE(core->IsRunning());
    
    core->Shutdown();
    EXPECT_FALSE(core->IsRunning());
    EXPECT_FALSE(core->IsInitialized());
}
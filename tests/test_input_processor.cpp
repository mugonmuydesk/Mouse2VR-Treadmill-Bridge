#include <gtest/gtest.h>
#include "core/InputProcessor.h"

using namespace Mouse2VR;

class InputProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<InputProcessor>();
    }
    
    std::unique_ptr<InputProcessor> processor;
};

TEST_F(InputProcessorTest, DefaultConfigValues) {
    ProcessingConfig config = processor->GetConfig();
    
    EXPECT_EQ(config.sensitivity, 1.0f);
    EXPECT_EQ(config.deadzone, 0.0f);
    EXPECT_FALSE(config.invertX);
    EXPECT_FALSE(config.invertY);
    EXPECT_FALSE(config.lockX);
    EXPECT_FALSE(config.lockY);
    EXPECT_EQ(config.maxSpeed, 1.0f);
}

TEST_F(InputProcessorTest, ProcessDeltaWithZero) {
    MouseDelta delta{0, 0};
    float outX, outY;
    
    processor->ProcessDelta(delta, 0.016f, outX, outY);  // 60 FPS delta time
    
    EXPECT_EQ(outX, 0.0f);
    EXPECT_EQ(outY, 0.0f);
}

TEST_F(InputProcessorTest, ProcessDeltaWithPositiveY) {
    // For treadmill, positive Y mouse movement = forward
    MouseDelta delta{0, 100};
    float outX, outY;
    
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    EXPECT_EQ(outX, 0.0f);  // No sideways movement
    EXPECT_GT(outY, 0.0f);   // Forward movement
}

TEST_F(InputProcessorTest, ProcessDeltaWithNegativeY) {
    MouseDelta delta{0, -100};
    float outX, outY;
    
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    EXPECT_EQ(outX, 0.0f);
    EXPECT_LT(outY, 0.0f);  // Backward movement
}

TEST_F(InputProcessorTest, SensitivityAffectsOutput) {
    MouseDelta delta{0, 100};
    float x1, y1, x2, y2;
    
    // Default sensitivity
    processor->ProcessDelta(delta, 0.016f, x1, y1);
    
    // Higher sensitivity
    ProcessingConfig config = processor->GetConfig();
    config.sensitivity = 2.0f;
    processor->SetConfig(config);
    processor->ProcessDelta(delta, 0.016f, x2, y2);
    
    EXPECT_GT(std::abs(y2), std::abs(y1));  // Higher sensitivity = more movement
}

TEST_F(InputProcessorTest, InvertYWorks) {
    MouseDelta delta{0, 100};
    float x1, y1, x2, y2;
    
    // Normal
    processor->ProcessDelta(delta, 0.016f, x1, y1);
    
    // Inverted
    ProcessingConfig config = processor->GetConfig();
    config.invertY = true;
    processor->SetConfig(config);
    processor->ProcessDelta(delta, 0.016f, x2, y2);
    
    EXPECT_EQ(y1, -y2);  // Should be opposite
}

TEST_F(InputProcessorTest, LockXPreventsHorizontalMovement) {
    MouseDelta delta{100, 100};  // Both X and Y movement
    float outX, outY;
    
    ProcessingConfig config = processor->GetConfig();
    config.lockX = true;  // Lock horizontal
    processor->SetConfig(config);
    
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    EXPECT_EQ(outX, 0.0f);  // X should be locked
    EXPECT_GT(outY, 0.0f);  // Y should work
}

TEST_F(InputProcessorTest, MaxSpeedClamps) {
    MouseDelta delta{0, 10000};  // Very large movement
    float outX, outY;
    
    ProcessingConfig config = processor->GetConfig();
    config.maxSpeed = 0.5f;  // Limit to half speed
    processor->SetConfig(config);
    
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    EXPECT_LE(std::abs(outY), 0.5f);  // Should be clamped
}

TEST_F(InputProcessorTest, CalibrationMode) {
    EXPECT_FALSE(processor->IsCalibrating());
    
    processor->StartCalibration();
    EXPECT_TRUE(processor->IsCalibrating());
    
    // Simulate some movement during calibration
    MouseDelta delta{0, 1000};
    float outX, outY;
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    // End calibration with distance
    processor->EndCalibration(1.0f);  // 1 meter
    EXPECT_FALSE(processor->IsCalibrating());
}

TEST_F(InputProcessorTest, SpeedCalculation) {
    // Initial speed should be zero
    EXPECT_EQ(processor->GetSpeedMetersPerSecond(), 0.0f);
    
    // Process some movement
    MouseDelta delta{0, 100};
    float outX, outY;
    processor->ProcessDelta(delta, 0.016f, outX, outY);
    
    // Speed should be non-zero now
    // (actual value depends on calibration)
}
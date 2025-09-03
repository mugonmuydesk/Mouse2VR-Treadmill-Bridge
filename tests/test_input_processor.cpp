#include <gtest/gtest.h>
#include "core/InputProcessor.h"

class InputProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<InputProcessor>();
    }
    
    std::unique_ptr<InputProcessor> processor;
};

TEST_F(InputProcessorTest, InitializeSucceeds) {
    EXPECT_TRUE(processor->Initialize());
}

TEST_F(InputProcessorTest, ProcessMouseInputWithZeroDelta) {
    EXPECT_TRUE(processor->Initialize());
    
    processor->ProcessMouseInput(0, 0);
    
    auto [x, y] = processor->GetProcessedValues();
    EXPECT_EQ(x, 0.0);
    EXPECT_EQ(y, 0.0);
}

TEST_F(InputProcessorTest, ProcessMouseInputWithPositiveDelta) {
    EXPECT_TRUE(processor->Initialize());
    
    processor->ProcessMouseInput(100, 50);
    
    auto [x, y] = processor->GetProcessedValues();
    EXPECT_GT(y, 0.0); // Forward movement (positive Y in mouse = forward in game)
}

TEST_F(InputProcessorTest, ProcessMouseInputWithNegativeDelta) {
    EXPECT_TRUE(processor->Initialize());
    
    processor->ProcessMouseInput(-100, -50);
    
    auto [x, y] = processor->GetProcessedValues();
    EXPECT_LT(y, 0.0); // Backward movement
}

TEST_F(InputProcessorTest, SetSensitivityAffectsOutput) {
    EXPECT_TRUE(processor->Initialize());
    
    // Test with sensitivity 1.0
    processor->SetSensitivity(1.0);
    processor->ProcessMouseInput(100, 100);
    auto [x1, y1] = processor->GetProcessedValues();
    
    // Test with sensitivity 2.0
    processor->SetSensitivity(2.0);
    processor->ProcessMouseInput(100, 100);
    auto [x2, y2] = processor->GetProcessedValues();
    
    // Higher sensitivity should produce larger values
    EXPECT_GT(std::abs(y2), std::abs(y1));
}

TEST_F(InputProcessorTest, ClampingWorks) {
    EXPECT_TRUE(processor->Initialize());
    
    // Test extreme values get clamped
    processor->ProcessMouseInput(10000, 10000);
    
    auto [x, y] = processor->GetProcessedValues();
    EXPECT_LE(std::abs(x), 1.0);
    EXPECT_LE(std::abs(y), 1.0);
}

TEST_F(InputProcessorTest, ResetClearsValues) {
    EXPECT_TRUE(processor->Initialize());
    
    processor->ProcessMouseInput(100, 100);
    auto [x1, y1] = processor->GetProcessedValues();
    EXPECT_NE(y1, 0.0);
    
    processor->Reset();
    auto [x2, y2] = processor->GetProcessedValues();
    EXPECT_EQ(x2, 0.0);
    EXPECT_EQ(y2, 0.0);
}
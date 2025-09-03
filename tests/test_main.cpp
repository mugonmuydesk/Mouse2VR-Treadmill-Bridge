#include <gtest/gtest.h>
#include <Windows.h>
#include <objbase.h>  // For CoInitializeEx

// Custom test environment for Windows-specific setup
class WindowsTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Initialize COM for tests that might need it
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    
    void TearDown() override {
        CoUninitialize();
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add Windows-specific test environment
    ::testing::AddGlobalTestEnvironment(new WindowsTestEnvironment);
    
    return RUN_ALL_TESTS();
}
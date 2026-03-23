#pragma once
#include <cstdint>
namespace imuReader
{
    #pragma pack(push, 1)
    struct IMUSample
    {
        IMUSample() : data(0,0,0), timestamp(0) {}
        IMUSample(float x, float y, float z, uint64_t nanosecond_timestamp) : data(x, y, z), timestamp(nanosecond_timestamp) {}
        float data[3];
        uint64_t timestamp;

    };
    #pragma pack(pop)

    enum IMUType : uint32_t
    {
        Accelerometer = 0,
        Gyroscope     = 1,
        Count         = 2
    };
}

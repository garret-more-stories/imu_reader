#pragma once
#include <cstdint>
namespace imuReader
{
    #pragma pack(push, 1)
    struct IMUSample
    {

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

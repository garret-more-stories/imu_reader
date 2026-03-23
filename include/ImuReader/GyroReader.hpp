#pragma once
#include "ImuReader/IMUCircularBuffer.hpp"

#include <cstdint>

#if defined(_WIN32)
    #define SENSOR_CALL_CONV __cdecl
    #ifdef IMU_LIB_EXPORTS
        #define IMU_API __declspec(dllexport)
    #else
        #define IMU_API __declspec(dllimport)
    #endif
#else
    #define SENSOR_CALL_CONV
    #define IMU_API __attribute__((visibility("default")))
#endif

// function pointer type for C# callbacks



#ifdef __cplusplus
extern "C" {
#endif
    
    typedef void(SENSOR_CALL_CONV *ControllerSensorCallback)(int controllerIndex, float x, float y, float z);
    
    IMU_API void register_gyro_callback       (ControllerSensorCallback callback);
    
    IMU_API void register_accel_callback      (ControllerSensorCallback callback);
        
    IMU_API bool set_controller_imu_state     (int controller_index, bool is_enabled);

    IMU_API const imuReader::IMUSample* return_imu_samples (int controller_index, imuReader::IMUType type);

    IMU_API const uint32_t return_samples_head             (int controller_index, imuReader::IMUType type);

    IMU_API const uint32_t return_samples_tail             (int controller_index, imuReader::IMUType type);
    
    IMU_API const uint32_t return_samples_capacity         (int controller_index, imuReader::IMUType type);

    IMU_API void start_sdl_loop    ();
 
    IMU_API void stop_sdl_loop     ();

    IMU_API int  return_number_two ();
    
#ifdef __cplusplus
}
#endif
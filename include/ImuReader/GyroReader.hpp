#pragma once

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

    IMU_API void change_polling_rate          (float polling_rate);
    
    IMU_API void register_gyro_callback       (ControllerSensorCallback callback);
    
    IMU_API void register_accel_callback      (ControllerSensorCallback callback);
        
    IMU_API bool set_controller_imu_state     (int controller_index, bool is_enabled);

    IMU_API void start_variable_rate_sdl_loop ();

    IMU_API void start_sdl_loop    ();
 
    IMU_API void stop_sdl_loop     ();

    IMU_API int  return_number_two ();
    
#ifdef __cplusplus
}
#endif
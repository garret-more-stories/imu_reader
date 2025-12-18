#pragma once
#if defined(_WIN32)
  #if defined(MYLIB_EXPORTS)
    #define IMU_API __declspec(dllexport)
  #else
    #define IMU_API __declspec(dllimport)
  #endif
#else
  #define IMU_API
#endif

// function pointer type for C# callbacks
typedef void(*ControllerSensorCallback)(int controllerIndex, float x, float y, float z);


extern "C"
{
    IMU_API void change_polling_rate     (float polling_rate);

    IMU_API void register_gyro_callback  (ControllerSensorCallback callback);

    IMU_API void register_accel_callback (ControllerSensorCallback callback);

    IMU_API void start_sdl_loop ();

    IMU_API void stop_sdl_loop  ();
}
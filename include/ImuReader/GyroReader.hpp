#pragma once

// function pointer type for C# callbacks
typedef void(*ControllerSensorCallback)(int controllerIndex, float x, float y, float z);


extern "C"
{
    void change_polling_rate     (float polling_rate);

    void register_gyro_callback  (ControllerSensorCallback callback);

    void register_accel_callback (ControllerSensorCallback callback);

    void start_sdl_loop ();

    void stop_sdl_loop  ();
}
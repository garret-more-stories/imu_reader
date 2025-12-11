#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>

// function pointer type for C# callbacks
typedef void(*ControllerSensorCallback)(int controllerIndex, float x, float y, float z);

static const std::unordered_set<u_int16_t> ignored_vendor_ids = 
{
    0x28DE
};

static std::thread        sdl_thread;
static std::mutex         controller_mutex;
static std::atomic<bool>  running          (false);
static std::atomic<float> poll_delay_ms    (1.f);

static std::atomic<ControllerSensorCallback>  gyro_callback    {nullptr}, 
                                              accel_callback   {nullptr};

// Might change this to a direct index look up structure with 2 arrays to optimize finding the ids
static std::vector<SDL_JoystickID>            controller_ids;

void add_gamepad(SDL_JoystickID id)
{
    // Controller must function properly in Unity for it to be added to the list
    // This creates parity between Unity's controller list and SDL's
    if(ignored_vendor_ids.count(SDL_GetGamepadVendorForID(id))) return; 

    SDL_Gamepad *gamepad_id = SDL_OpenGamepad(id);
    if(!gamepad_id) return; 

    controller_ids.push_back(SDL_GetGamepadID(gamepad_id)); 

    if (SDL_GamepadHasSensor(gamepad_id, SDL_SENSOR_ACCEL) && SDL_GamepadHasSensor(gamepad_id, SDL_SENSOR_GYRO)) 
    {
        bool sensorState = SDL_SetGamepadSensorEnabled(gamepad_id, SDL_SENSOR_GYRO , true);
        bool accelState  = SDL_SetGamepadSensorEnabled(gamepad_id, SDL_SENSOR_ACCEL, true);
    }
}

void run_sdl_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) 
    {
        switch (event.type) 
        {     
          case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
          {
            ControllerSensorCallback callback;
            switch (event.gsensor.sensor) 
            {
              case SDL_SENSOR_GYRO:
                callback = gyro_callback.load  (std::memory_order_acquire);
                break;
              case SDL_SENSOR_ACCEL:
                callback = accel_callback.load (std::memory_order_acquire);
                break;
            }
            if(callback)
            {  
                auto iterator = std::lower_bound(controller_ids.begin(), controller_ids.end(), event.gsensor.which);
                if(iterator != controller_ids.end())
                {
                    callback(std::distance(controller_ids.begin(), iterator), event.gsensor.data[0], event.gsensor.data[1], event.gsensor.data[2]); ;
                }
                
            }
            break;
          }
      
          case SDL_EVENT_GAMEPAD_REMOVED:
          {
            std::lock_guard<std::mutex> guard(controller_mutex);
        
            auto iterator = std::lower_bound(controller_ids.begin(), controller_ids.end(), event.gdevice.which);
            // If this condition is met then it most likely an invalid gamepad being removed so it doesn't affect our structure
            if(iterator != controller_ids.end() && *iterator == event.gdevice.which)
            {
                controller_ids.erase(iterator);
            }
            break;
          }
      
          case SDL_EVENT_GAMEPAD_ADDED:
          {
            std::lock_guard<std::mutex> guard(controller_mutex);
        
            add_gamepad(event.gdevice.which);
            break;
          }
        }
    }
}

extern "C"
{
    void change_polling_rate(float polling_rate)
    {
        poll_delay_ms = 1000.f/polling_rate;
    }

    void register_gyro_callback(ControllerSensorCallback callback)
    {
        gyro_callback.store(callback, std::memory_order_release);
    }

    void register_accel_callback(ControllerSensorCallback callback)
    {
        accel_callback.store(callback, std::memory_order_release);
    }

    void start_sdl_loop()
    {
        if (running) return;

        running    = true;   
        sdl_thread = std::thread([]() 
        {
            SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_SENSOR); 
            while (running) 
            {
                run_sdl_loop();
                std::this_thread::sleep_for(std::chrono::milliseconds(poll_delay_ms));
            } 
            
            controller_ids.clear();
            SDL_Quit();
        });
    }   
    void stop_sdl_loop() 
    {
        running = false;   

        if (sdl_thread.joinable())
        {
          sdl_thread.join();  
        }  
    }
}

void notify_gyro(int controller, float x, float y, float z)
{
    printf("Gyro from controller %i: %f, %f, %f\n",controller, x, y, z);
}

void notify_accel(int controller, float x, float y, float z)
{
    printf("Accel from controller %i: %f, %f, %f\n",controller, x, y, z);
}
void test_gyro()
{
    start_sdl_loop          ();
    register_gyro_callback  (&notify_gyro);
    register_accel_callback (&notify_accel);
    while (running) 
    {   
    }
}

#ifndef BUILDING_SO
int main(int argc, char **argv) 
{
    test_gyro();
    return 0;
}
#endif
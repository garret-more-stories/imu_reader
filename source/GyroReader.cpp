#include <ImuReader/GyroReader.hpp>

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <iostream>

namespace imuReader
{
    static std::unordered_set<uint16_t> ignored_vendor_ids = 
    {
        0x28DE // Steam Deck
    };

    static std::thread        sdl_thread;
    static std::mutex         controller_mutex;
    static std::atomic<bool>  running          (false);
    static std::atomic<float> poll_delay_ms    (1.f);

    // Optional callback to process individual IMU readings as they come
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
        if(SDL_WaitEvent(&event))
        {
            do
            {
                switch (event.type) 
                {
                    case SDL_EVENT_QUIT: 
                        return;

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
                                std::cout << "SDL Time:" << event.gsensor.sensor_timestamp << " ";
                                callback(static_cast<int>(std::distance(controller_ids.begin(), iterator)), 
                                         event.gsensor.data[0], 
                                         event.gsensor.data[1], 
                                         event.gsensor.data[2]);
                            } 
                        }
                        break;
                    }   

                    case SDL_EVENT_GAMEPAD_REMOVED:
                    {
                        std::lock_guard<std::mutex> guard(controller_mutex);  
                        auto iterator = std::lower_bound(controller_ids.begin(), controller_ids.end(), event.gdevice.which);
                        // If this condition is not met then it most likely an invalid gamepad being removed so it doesn't affect our structure
                        if(iterator != controller_ids.end() && *iterator == event.gdevice.which)
                        {
                            controller_ids.erase(iterator);
                            SDL_CloseGamepad(SDL_GetGamepadFromID(event.gdevice.which));
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
            while (SDL_PollEvent(&event));
        }
    }
} 

#pragma region Extern C methods

void change_polling_rate(float polling_rate)
{
    imuReader::poll_delay_ms.store(1000.f/polling_rate, std::memory_order_relaxed);
}

void register_gyro_callback(ControllerSensorCallback callback)
{
    imuReader::gyro_callback.store(callback, std::memory_order_release);
}

void register_accel_callback(ControllerSensorCallback callback)
{
    imuReader::accel_callback.store(callback, std::memory_order_release);
}

bool set_controller_imu_state (int controller_index, bool is_enabled)
{
    std::lock_guard<std::mutex> guard(imuReader::controller_mutex);

    auto gamepad_id = SDL_GetGamepadFromID(imuReader::controller_ids[controller_index]);

    return gamepad_id && 
           SDL_GamepadHasSensor        (gamepad_id, SDL_SENSOR_GYRO ) &&  
           SDL_GamepadHasSensor        (gamepad_id, SDL_SENSOR_ACCEL) && 
           SDL_SetGamepadSensorEnabled (gamepad_id, SDL_SENSOR_GYRO , is_enabled) && 
           SDL_SetGamepadSensorEnabled (gamepad_id, SDL_SENSOR_ACCEL, is_enabled) ;
}

void stop_sdl_loop() 
{
    imuReader::running.store(false, std::memory_order_release);   

    SDL_Event quit_event;
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);

    if (imuReader::sdl_thread.joinable())
    {
        imuReader::sdl_thread.join();  
    }  
}

void start_sdl_loop()
{
    stop_sdl_loop();

    imuReader::controller_ids.clear();
    imuReader::running.store(true, std::memory_order_relaxed);   

    imuReader::sdl_thread = std::thread([]() 
    {
        SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_SENSOR); 
        while (imuReader::running.load(std::memory_order_relaxed)) 
        {
            imuReader::run_sdl_loop();                           
        } 

        SDL_Quit();
    });
}  

int return_number_two()
{
    return 2;
}

#pragma endregion
 

#include "ImuReader/GyroReader.hpp"
#include "ImuReader/IMUController.hpp"

#include <vector>
#include <algorithm>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <memory>


namespace imuReader
{

    static IMUCircularBuffer imuBuffers[IMUType::Count];

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
    static std::vector<std::unique_ptr<IMUController>>            controllers;
    
    void add_gamepad(SDL_JoystickID id)
    {
        // Controller must function properly in Unity for it to be added to the list
        // This creates parity between Unity's controller list and SDL's
        if(ignored_vendor_ids.count(SDL_GetGamepadVendorForID(id))) return; 

        SDL_Gamepad *gamepad_id = SDL_OpenGamepad(id);
        if(!gamepad_id) return; 

        controllers.push_back(std::make_unique<IMUController>(SDL_GetGamepadID(gamepad_id))); 

        if (SDL_GamepadHasSensor(gamepad_id, SDL_SENSOR_ACCEL) && SDL_GamepadHasSensor(gamepad_id, SDL_SENSOR_GYRO)) 
        {
            bool sensor_state = SDL_SetGamepadSensorEnabled(gamepad_id, SDL_SENSOR_GYRO , true);
            bool accel_state  = SDL_SetGamepadSensorEnabled(gamepad_id, SDL_SENSOR_ACCEL, true);
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
                        IMUType imu_type;

                        auto iterator = std::lower_bound(
                        controllers.begin(), 
                        controllers.end(), 
                        event.gsensor.which,
                        [](const std::unique_ptr<IMUController>& ptr, SDL_JoystickID id) { return ptr->id < id; });

                        

                        switch (event.gsensor.sensor) 
                        {
                            case SDL_SENSOR_GYRO:
                                callback = gyro_callback.load  (std::memory_order_acquire);
                                imu_type = IMUType::Gyroscope;
                                break;
                            case SDL_SENSOR_ACCEL:
                                callback = accel_callback.load (std::memory_order_acquire);
                                imu_type = IMUType::Accelerometer;
                                break;
                        }
                        
                        if(iterator != controllers.end())
                        {
                            auto index = static_cast<int>(std::distance(controllers.begin(), iterator));

                           controllers[index]->imuBuffers[imu_type].
                            push(IMUSample( event.gsensor.data[0], 
                                            event.gsensor.data[1], 
                                            event.gsensor.data[2], 
                                            event.gsensor.sensor_timestamp)); 

                            if(callback)
                            {  

                                std::cout << "SDL Time:" << event.gsensor.sensor_timestamp << " ";
                                callback(index, 
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
                        auto iterator = std::lower_bound(
                            controllers.begin(), 
                            controllers.end(), 
                            event.gdevice.which, 
                            [](const std::unique_ptr<IMUController>& ptr, SDL_JoystickID id) { return ptr->id < id; });

                        // If this condition is not met then it most likely an invalid gamepad being removed so it doesn't affect our structure
                        if(iterator != controllers.end() && iterator->get()->id == event.gdevice.which)
                        {
                            controllers.erase(iterator);
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

    auto gamepad_id = SDL_GetGamepadFromID(imuReader::controllers[controller_index].get()->id);

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

    imuReader::controllers.clear();
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

const imuReader::IMUSample* return_imu_samples(int controller_index, imuReader::IMUType type)
{
   return imuReader::controllers[controller_index].get()->imuBuffers[type].get_data();
}
const uint32_t return_samples_head(int controller_index, imuReader::IMUType type)
{
    return imuReader::controllers[controller_index].get()->imuBuffers[type].get_head();
}
const uint32_t return_samples_tail(int controller_index, imuReader::IMUType type)
{
    return imuReader::controllers[controller_index].get()->imuBuffers[type].get_tail();
}
const uint32_t return_samples_capacity(int controller_index, imuReader::IMUType type)
{
    return imuReader::IMUCircularBuffer::IMU_CAPACITY;
}

#pragma endregion
 

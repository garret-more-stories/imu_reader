#pragma once
#include "IMUCircularBuffer.hpp"

#include <SDL3/SDL.h>

namespace imuReader
{
    struct IMUController
    {
        IMUController() {}
        IMUController(SDL_JoystickID joystick_id) : id(joystick_id) {}

        IMUCircularBuffer imuBuffers[IMUType::Count];
        SDL_JoystickID    id;
    };
}

#include <ImuReader/GyroReader.hpp>
#include <iostream>
#include <thread>
#include <chrono>

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

    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto disabled = set_controller_imu_state(0, false);
    std::cout << disabled << "\n";
    while (true) 
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
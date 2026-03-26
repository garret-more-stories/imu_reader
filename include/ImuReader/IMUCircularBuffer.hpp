#pragma once
#include <cstdint>
#include <atomic>
#include <stdalign.h>
#include <cassert>
#include "IMUSample.hpp"

namespace imuReader
{
    // Single Producer Single Consumer Circular Buffer meant to be used by Unity's input thread exclusively
    // Not meant for general use
    class IMUCircularBuffer
    {
    
    public:

        // A capacity for the maximum expected number of IMU values per check
        static const uint32_t IMU_CAPACITY = 256;

        IMUCircularBuffer() : capacity(IMU_CAPACITY)
        {

        }

        void push(const IMUSample& value)
        {
            uint32_t last_head = head.value.load(std::memory_order_relaxed);
            uint32_t next      = (last_head + 1) & (capacity - 1);
            
            if(next == tail.value.load(std::memory_order_acquire))
            {
                tail.value.store((tail.value.load(std::memory_order_acquire)+ 1) & (capacity - 1), std::memory_order_release);
            }
    
            data[last_head] = value;
            head.value.store(next, std::memory_order_release);
        }
    
        uint32_t get_tail()     const { return tail.value.load(std::memory_order_acquire); }
        uint32_t get_head()     const { return head.value.load(std::memory_order_acquire); }

        void update_tail(uint32_t new_tail) 
        {
            tail.value.store(new_tail, std::memory_order_release);
        }

        const IMUSample* get_data()     const { return data; }
    
    private:
        struct alignas(64) PaddedAtomic
        {
            std::atomic<uint32_t> value;
        };

        PaddedAtomic head{0};
        PaddedAtomic tail{0};
        //uint32_t     cached_tail {0};
        uint32_t     capacity;
        alignas(64) IMUSample data [IMU_CAPACITY];
    };
}
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

        IMUCircularBuffer(uint32_t buffer_capacity) : capacity(buffer_capacity)
        {
            assert(("", (capacity & (capacity - 1)) == 0));
        }
        void Push(const IMUSample& value)
        {
            uint32_t last_head = head.value.load(std::memory_order_relaxed);
            uint32_t next      = (last_head + 1) & (capacity - 1);
            
            if(next ==  cached_tail && 
               next == (cached_tail = tail.value.load(std::memory_order_acquire))) 
            {
                tail.value.store(cached_tail = (cached_tail + 1) & (capacity - 1), std::memory_order_release);
            }
    
            if (next == tail.value.load(std::memory_order_acquire))
            {
                tail.value.store((tail.value.load(std::memory_order_relaxed) + 1) & (capacity - 1),
                           std::memory_order_release);
            }
    
            data[last_head] = value;
            head.value.store(next, std::memory_order_release);
        }
    
        uint32_t GetTail()     const { return tail.value.load(std::memory_order_acquire); }
        uint32_t GetHead()     const { return head.value.load(std::memory_order_acquire); }
        uint32_t GetCapacity() const { return capacity; }

        const IMUSample* GetData()     const { return data; }
    
    private:
        struct alignas(64) PaddedAtomic
        {
            std::atomic<uint32_t> value;
        };

        PaddedAtomic head{0};
        PaddedAtomic tail{0};
        uint32_t     cached_tail {0};
        uint32_t     capacity;
        alignas(64) IMUSample* data;
    };
}
// A custom multi-thread safe circular buffer, built for performance and memory efficiency
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <cstdint>
#include <atomic>
#include <array>

/**
 * @brief The CircularBuffer class is a custom multi-thread safe circular buffer, built for performance and memory efficiency
 */
template <typename T, size_t Size>
class CircularBuffer
{
    // enforce that buffer size is a power of 2, enabling use of bitwise masking to wrap around buffer
    // For powers of 2, the binary representation when compared with the number below it using bitwise AND should result in zero
    // e.g. 8 (1000) & 7 (0111) = 0
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

private:
    // Use a regular array to store the data - arrays are contiguous in memory
    std::array<T, Size> buffer;

    // use atomic integers to track our read/write indices - start at zero
    // atomics are multi-threading safe, helps to avoid collisions between multiple threads
    std::atomic<size_t> read_index{0};
    std::atomic<size_t> write_index{0};

    // Bit mask that allows us to short-hand wrap around the buffer
    // 'Mask' in this contect refers to defining what bits we want to keep/not keep
    size_t Mask = Size - 1;

public:
    /**
     * @brief Attempts to push an item to the buffer, failing if buffer is full
     * @param value The reference to the value to push to the buffer, made const to protect the value
     * @return True if push was successful, false if failed to push to buffer
     */
    bool try_push(const T &value)
    {
        // get current write index, by loading the current value of the atomic
        // The memory_order_relaxed parameter is used to indicate that the operation does not need to be synchronised with other threads
        size_t current_write = write_index.load(std::memory_order_relaxed);
        // calculate next write index, aplying bit mask to wrap around buffer if necessary
        size_t next_write = (current_write + 1) & Mask;
        std::cout << "Next write index: " << next_write << std::endl;

        // check if buffer is full - if next write index is equal to read index, buffer is full (i.e. we reached end of buffer)
        if (next_write == read_index.load(std::memory_order_acquire))
        {
            // buffer is full
            return false;
        }

        // store new value
        buffer[current_write] = value;
        // update write index
        write_index.store(next_write, std::memory_order_release);
        std::cout << "Pushed value to buffer at index " << current_write << std::endl;
        return true;
    }

    /**
     * @brief Attempt to read an item from the buffer and store into the provided value
     * @param value The reference to the value to store the read value
     * @return True if read was successful, false if failed to read from buffer
     */
    bool try_pop(T &value)
    {
        // get current read index
        size_t current_read = read_index.load(std::memory_order_relaxed);
        // check if buffer is empty - if read index is equal to write index, buffer is empty
        if (current_read == write_index.load(std::memory_order_relaxed))
        {
            // buffer is empty
            return false;
        }

        // read value from buffer
        value = buffer[current_read];
        // calculate next read index, applying bit mask to wrap around buffer if necessary
        size_t next_read = (current_read + 1) & Mask;
        // update read index
        read_index.store(next_read, std::memory_order_relaxed);
        return true;
    }

    /**
     * @brief Get the number of items currently in the buffer
     * @return The number of items in the buffer
     */
    size_t size() const
    {
        // (write index - read index) gives the number of items in the buffer
        size_t current_write = write_index.load(std::memory_order_relaxed);
        size_t current_read = read_index.load(std::memory_order_relaxed);
        return (current_write - current_read) & Mask;
    }

    /**
     * @brief Check if buffer is empty
     * @return True if buffer is empty, false if not empty
     */
    bool empty() const
    {
        // buffer is empty if read index is equal to write index
        return size() == 0;
    }

    /**
     * @brief Get max size of the buffer
     * @return The max size of the buffer
     */
    constexpr size_t max_size() const
    {
        // Note using constexpr to indicate that this function is evaluated at compile time, improving performance
        return Size;
    }
};

#endif
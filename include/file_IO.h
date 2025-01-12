#ifndef FILE_IO_H
#define FILE_IO_H

#include <fstream>
#include <iostream>
#include <sstream>

/**
 * Helper class to manage file I/O operations
 */
class FileIO
{
public:
    /**
     * @brief Read a file into a string
     * @param filename The name of the file to read
     * @return The contents of the file as a string
     */
    static std::string read_file(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << filename << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    /**
     * @brief Write a string to a file
     * @param filename The name of the file to write to
     * @param data The data to write to the file
     */
    static void write_file(const std::string &filename, const std::string &data)
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        file << data;
    }

    /**
     * @brief Append a string to a file
     * @param filename The name of the file to append to
     * @param data The data to append to the file
     */
    static void append_to_file(const std::string &filename, const std::string &data)
    {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        file << data;
    }
};

#endif // FILE_IO_H
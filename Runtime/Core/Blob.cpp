#include "Blob.h"
#include "Log.h"
#include <fstream>

bool Blob::ReadBinaryFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if(!file.is_open())
    {
        Log::Error("File %s dose not exits or is locked", path.string().c_str());
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    m_Size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_Data = static_cast<uint8_t*>(malloc(m_Size));
    if (m_Data == nullptr)
    {
        Log::Error("Failed to malloc memory for the file %s", path.string().c_str());
        m_Data = nullptr;
        m_Size = 0;
        return false;
    }

    file.read(reinterpret_cast<char*>(m_Data), m_Size);

    if (!file.good())
    {
        Log::Error("Read file %s error", path.string().c_str());
        m_Data = nullptr;
        m_Size = 0;
        return false;
    }

    return true;
}

void Blob::Release()
{
    if(m_Data)
    {
        free(m_Data);
        m_Data = nullptr;
    }
    m_Size = 0;
}
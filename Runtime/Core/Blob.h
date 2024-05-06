#pragma once
#include <filesystem>

class Blob
{
public:
    Blob() : m_Data(nullptr), m_Size(0) {}
    ~Blob() { Release(); }
    Blob(const Blob&) = delete;
    Blob(Blob&&) = delete;
    Blob& operator=(const Blob&) = delete;
    Blob& operator=(Blob&&) = delete;

    const uint8_t*  GetData() const { return m_Data; }
    size_t          GetSize() const { return m_Size; }
    bool            IsEmpty() const { return m_Data == nullptr || m_Size == 0; }
    bool            ReadBinaryFile(const std::filesystem::path& path);
    void            Release();  

private:
    uint8_t* m_Data;
    size_t m_Size;
};
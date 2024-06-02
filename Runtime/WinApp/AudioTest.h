#pragma once
#include "../Core/Win32Base.h"
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <wrl/client.h>

class AudioTest : public Win32Base
{
public:
    using Win32Base::Win32Base;

    bool PlayAudio();
    bool StopAudio();
    bool PauseAudio();

protected:
    bool Init() final;
    void Tick()  final;
    void Shutdown() final;
    void OnBeginResize() override;
    void OnResize(int width, int height) final;
    void OnEndResize() final;
    
private:
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_DeviceEnumerator;
    Microsoft::WRL::ComPtr<IMMDeviceCollection> m_DeviceCollection;
    Microsoft::WRL::ComPtr<IMMDevice> m_Device;
    Microsoft::WRL::ComPtr<IAudioClient> m_AudioClient;
    Microsoft::WRL::ComPtr<IAudioRenderClient> m_AudioRenderClient;

    WAVEFORMATEX * m_Pwfx = NULL;
    uint32_t m_BufferFrameCount;
    uint32_t m_DeviceCount;
};

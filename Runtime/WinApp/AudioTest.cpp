#include "AudioTest.h"
#include "Functiondiscoverykeys_devpkey.h"
#include "../Core/Log.h"
#include "../Core/Misc.h"

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

static void LogoutDeviceInfo(const Microsoft::WRL::ComPtr<IMMDevice>& tempDevice)
{
    Microsoft::WRL::ComPtr<IPropertyStore> deviceProperty;
    HRESULT hr = tempDevice->OpenPropertyStore(STGM_READ, &deviceProperty);

    LPWSTR pwszID;
    tempDevice->GetId(&pwszID);
    
    PROPVARIANT varName;
    // Initialize container for property value.
    PropVariantInit(&varName);

    // Get the endpoint's friendly-name property.
    hr = deviceProperty->GetValue(
                   PKEY_Device_FriendlyName, &varName);

    // GetValue succeeds and returns S_OK if PKEY_Device_FriendlyName is not found.
    // In this case vartName.vt is set to VT_EMPTY.
    std::wstring wstr = StringFormat(L"Endpoint: \"%s\" (%s)", varName.pwszVal, pwszID);
    if (varName.vt != VT_EMPTY)
    {
        // Print endpoint friendly name and endpoint ID.
        Log::Message(Log::ELogLevel::Info, wstr.c_str());
    }
    
    CoTaskMemFree(pwszID);
    pwszID = NULL;
    PropVariantClear(&varName);

    // hr = deviceProperty->GetValue(
    //                PKEY_AudioEngine_DeviceFormat, &formFactor);

    // if(formFactor.uintVal == Headphones)
    // {
    //     Log::Info("Headphones");
    // }
    // else if(formFactor.uintVal == Microphone)
    // {
    //     Log::Info("Microphone");
    // }
    // else if(formFactor.uintVal == Speakers)
    // {
    //     Log::Info("Speakers");
    // }
    // else if(formFactor.uintVal == Handset)
    // {
    //     Log::Info("Handset");
    // }
    // else if(formFactor.uintVal == DigitalAudioDisplayDevice)
    // {
    //     Log::Info("DigitalAudioDisplayDevice");
    // }
    // else 
    // {
    //     Log::Info("Unknown form factor");
    // }
}

bool AudioTest::Init()
{
    CoInitialize(NULL);
    
    HRESULT hr = CoCreateInstance( __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&m_DeviceEnumerator));
    if(FAILED(hr))
    {
        Log::Error("Failed to create IMMDeviceEnumerator");
        return false;
    }

    hr = m_DeviceEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &m_DeviceCollection);
    if(FAILED(hr))
    {
        Log::Error("Failed to enum audio endpoints");
        return false;
    }

    hr = m_DeviceCollection->GetCount(&m_DeviceCount);

    for(uint32_t i = 0; i < m_DeviceCount; i++)
    {
        Microsoft::WRL::ComPtr<IMMDevice> tempDevice;
        hr = m_DeviceCollection->Item(i, &tempDevice);

        LogoutDeviceInfo(tempDevice);

        Microsoft::WRL::ComPtr<IMMEndpoint> tempEndPoint;
        hr = tempDevice->QueryInterface(IID_PPV_ARGS(&tempEndPoint));
        
    }

    hr = m_DeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_Device);
    Log::Info("Selected default audio endpoint");
    LogoutDeviceInfo(m_Device);

    if(m_Device != nullptr)
    {
        hr = m_Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, &m_AudioClient);

        if(FAILED(hr))
        {
            Log::Error("Failed to active audio client");
            return false;
        }
    }

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    
    hr = m_AudioClient->GetMixFormat(&m_Pwfx);
    if(FAILED(hr))
    {
        Log::Error("Failed to get mix format");
        return false;
    }

    hr = m_AudioClient->Initialize(
                         AUDCLNT_SHAREMODE_SHARED,
                         0,
                         hnsRequestedDuration,
                         0,
                         m_Pwfx,
                         NULL);
    
    hr = m_AudioClient->GetBufferSize(&m_BufferFrameCount);

    hr = m_AudioClient->GetService(IID_PPV_ARGS(&m_AudioRenderClient));

    if(FAILED(hr))
    {
        Log::Error("Failed to get audio render client");
        return false;
    }
    
    return true;
}

bool AudioTest::PlayAudio()
{
    HRESULT hr = m_AudioClient->Start();


    
    return true;
}

bool AudioTest::StopAudio()
{
    HRESULT hr = m_AudioClient->Stop();
    return true;
}

bool AudioTest::PauseAudio()
{
    
    return true;
}

void AudioTest::Tick()
{
    
}

void AudioTest::Shutdown()
{
    CoTaskMemFree(m_Pwfx);
    CoUninitialize();
}

void AudioTest::OnBeginResize()
{
    
}

void AudioTest::OnResize(int width, int height)
{
    
}

void AudioTest::OnEndResize()
{
    
}
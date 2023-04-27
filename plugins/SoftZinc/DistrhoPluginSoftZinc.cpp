/*
 * DISTRHO Zinc
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "extra/Mutex.hpp"

#define DPF_JACK_STANDALONE_SKIP_RTAUDIO_FALLBACK
#define DPF_JACK_STANDALONE_SKIP_SDL2_FALLBACK
#define JACKBRIDGE_SKIP_NATIVE_UTILS
#include "src/jackbridge/JackBridge.cpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class DistrhoPluginSoftZinc : public Plugin
{
    static constexpr const uint16_t kMaxBufferSize = 8192;

public:
    DistrhoPluginSoftZinc()
        : Plugin(0, 0, 0)
    {
        if (isDummyInstance())
            return;

        client = jackbridge_client_open("Soft Zinc", JackNoStartServer, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(client != nullptr,);

        ports[0] = jackbridge_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        ports[1] = jackbridge_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        jackbridge_set_process_callback(client, _JackProcessCallback, this);

        recv1 = new float[kMaxBufferSize];
        recv2 = new float[kMaxBufferSize];
    }

    ~DistrhoPluginSoftZinc()
    {
        if (client == nullptr)
            return;

        jackbridge_client_close(client);

        delete[] recv1;
        delete[] recv2;
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "SoftZinc";
    }

    const char* getDescription() const override
    {
        return "Utility plugin for getting sound out of plugin hosts into JACK.";
    }

    const char* getMaker() const noexcept override
    {
        return "falkTX";
    }

    const char* getHomePage() const override
    {
        return "https://github.com/DISTRHO/Zinc";
    }

    const char* getLicense() const noexcept override
    {
        return "ISC";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('S', 'Z', 'n', 'c');
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Init

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo;

        Plugin::initAudioPort(input, index, port);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Process

    void activate() override
    {
        if (client == nullptr)
            return;

        filled = 0;
        jackbridge_activate(client);
    }

    void deactivate() override
    {
        if (client == nullptr)
            return;

        jackbridge_deactivate(client);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (inputs[0] != outputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);

        if (inputs[1] != outputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);

        if (client != nullptr)
        {
            const MutexLocker cml(recvlock);

            DISTRHO_SAFE_ASSERT_RETURN(filled + frames < kMaxBufferSize,);

            std::memcpy(recv1 + filled, inputs[0], sizeof(float) * frames);
            std::memcpy(recv2 + filled, inputs[1], sizeof(float) * frames);

            filled += frames;
        }
    }

private:
    // jack stuff
    jack_client_t* client = nullptr;
    jack_port_t* ports[2];

    // audio received from plugin
    float* recv1 = nullptr;
    float* recv2 = nullptr;
    uint16_t filled = 0;
    Mutex recvlock;

    static int _JackProcessCallback(const uint32_t frames, void* const arg)
    {
        static_cast<DistrhoPluginSoftZinc*>(arg)->JackProcessCallback(frames);
        return 0;
    }

    void JackProcessCallback(const uint32_t frames)
    {
        float* const buf1 = (float*)jackbridge_port_get_buffer(ports[0], frames);
        float* const buf2 = (float*)jackbridge_port_get_buffer(ports[1], frames);

        {
            const MutexLocker cml(recvlock);

            if (filled < frames)
            {
                std::memset(buf1, 0, sizeof(float) * frames);
                std::memset(buf2, 0, sizeof(float) * frames);
            }
            else
            {
                std::memcpy(buf1, recv1, sizeof(float) * frames);
                std::memcpy(buf2, recv2, sizeof(float) * frames);

                std::memmove(recv1, recv1 + frames, sizeof(float) * frames);
                std::memmove(recv2, recv1 + frames, sizeof(float) * frames);

                filled -= frames;
            }
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPluginSoftZinc)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginSoftZinc();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

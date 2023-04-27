/*
 * DISTRHO Zinc
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "Semaphore.hpp"

#define DPF_JACK_STANDALONE_SKIP_RTAUDIO_FALLBACK
#define DPF_JACK_STANDALONE_SKIP_SDL2_FALLBACK
#define JACKBRIDGE_SKIP_NATIVE_UTILS
#include "src/jackbridge/JackBridge.cpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class DistrhoPluginHardZinc : public Plugin
{
public:
    DistrhoPluginHardZinc()
        : Plugin(0, 0, 0)
    {
        if (isDummyInstance())
            return;

        client = jackbridge_client_open("Hard Zinc", JackNoStartServer, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(client != nullptr,);

        buffersize = jackbridge_get_buffer_size(client);

        ports[0] = jackbridge_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        ports[1] = jackbridge_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        jackbridge_set_process_thread(client, _JackThreadCallback, this);
    }

    ~DistrhoPluginHardZinc()
    {
        if (client == nullptr)
            return;

        jackbridge_client_close(client);
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "HardZinc";
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
        return d_cconst('H', 'Z', 'n', 'c');
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

        jackbridge_activate(client);
    }

    void deactivate() override
    {
        if (client == nullptr)
            return;

        inputsSrc = nullptr;
        sem1.post();
        sem1.wait();
        jackbridge_deactivate(client);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(frames == buffersize,);
        DISTRHO_SAFE_ASSERT_RETURN(client != nullptr,);

        inputsSrc = inputs;
        sem1.post();

        if (inputs[0] != outputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*buffersize);

        if (inputs[1] != outputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float)*buffersize);

        sem2.wait();
    }

private:
    // jack stuff
    jack_client_t* client = nullptr;
    jack_port_t* ports[2];
    uint32_t buffersize = 0;

    // direct plugin buffer pointer
    const float** inputsSrc = nullptr;

    // semaphores for sync
    Semaphore sem1;
    Semaphore sem2;

    static void* _JackThreadCallback(void* arg)
    {
        static_cast<DistrhoPluginHardZinc*>(arg)->JackThreadCallback();
        return nullptr;
    }

    void JackThreadCallback()
    {
        for (;;)
        {
            const uint32_t frames = jackbridge_cycle_wait(client);
            DISTRHO_SAFE_ASSERT_RETURN(frames != 0,);
            DISTRHO_SAFE_ASSERT_RETURN(frames == buffersize,);

            float* bufs[] = {
                (float*)jackbridge_port_get_buffer(ports[0], frames),
                (float*)jackbridge_port_get_buffer(ports[1], frames),
            };

            sem1.wait();

            if (const float** inputs = inputsSrc)
            {
                std::memcpy(bufs[0], inputs[0], sizeof(float) * buffersize);
                std::memcpy(bufs[1], inputs[1], sizeof(float) * buffersize);
                sem2.post();
            }
            else
            {
                std::memset(bufs[0], 0, sizeof(float) * buffersize);
                std::memset(bufs[1], 0, sizeof(float) * buffersize);
            }

            jackbridge_cycle_signal(client, 0);
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPluginHardZinc)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginHardZinc();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

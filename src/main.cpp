#include <stdio.h>
#include <string.h>
#include <string>
#include "daisysp.h"
#include "daisy_field.h"

using namespace daisysp;
using namespace daisy;

static DaisyField df;
static Metro      tick;

static Oscillator car1;
static Oscillator car2;
// static Oscillator mod1;
// static Oscillator mod2;
static BlOsc mod1;
static BlOsc mod2;

static Parameter car1FreqParam;
static Parameter mod1FreqParam;
static Parameter index1Param;
static Parameter car2FreqParam;
static Parameter mod2FreqParam;
static Parameter index2Param;

static ReverbSc   verb;
static Parameter verbAmountParam;

static Fold       fold;
static Parameter foldIncrParam;

static Limiter limiter;

uint8_t sumbuff[1024];

void UsbCallback(uint8_t* buf, uint32_t* len)
{
    for(size_t i = 0; i < *len; i++)
    {
        sumbuff[i] = buf[i];
    }
}

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig, verb1, verb2, verbAmount;
    for(size_t i = 0; i < size; i += 2)
    {
        car1.SetFreq(car1FreqParam.Process() + (mod1.Process() * index1Param.Process() * mod1FreqParam.Process()));
        car2.SetFreq(car2FreqParam.Process() + (mod2.Process() * index2Param.Process() * mod2FreqParam.Process()));

        sig = car1.Process();
        sig += car2.Process();

        sig = fold.Process(sig);

        verb.Process(sig, sig, &verb1, &verb2);

        verbAmount = verbAmountParam.Process();
        sig = sig * 1 - verbAmount;

        out[i] = sig + (verb1 * verbAmount);
        out[i + 1] = sig + (verb2 * verbAmount);
    }

    limiter.ProcessBlock(out, size, 1);
}

void PrintSerial(std::string msg)
{
    msg = msg + "\r\n";
    df.seed.usb_handle.TransmitInternal((uint8_t*)msg.c_str(), msg.length());
    dsy_system_delay(5);
}

void PrintOled(uint8_t x, uint8_t y, std::string str) {
    df.display.SetCursor(x, y);
    char* cstr = &str[0];
    df.display.WriteString(cstr, Font_7x10, true);
}

int main(void)
{
    df.Init();

    df.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
    df.seed.usb_handle.SetReceiveCallback(UsbCallback, UsbHandle::FS_INTERNAL);

    float sample_rate;
    sample_rate = df.seed.AudioSampleRate();

    tick.Init(100.0, sample_rate);

    car1.Init(sample_rate);
    mod1.Init(sample_rate);

    car2.Init(sample_rate);
    mod2.Init(sample_rate);

    car1FreqParam.Init(*df.GetKnob(DaisyField::KNOB_1), 1, 6000, Parameter::EXPONENTIAL);
    mod1FreqParam.Init(*df.GetKnob(DaisyField::KNOB_2), 1, 6000, Parameter::EXPONENTIAL);
    index1Param.Init(*df.GetKnob(DaisyField::KNOB_3), 0, 10, Parameter::EXPONENTIAL);

    car2FreqParam.Init(*df.GetKnob(DaisyField::KNOB_4), 1, 6000, Parameter::EXPONENTIAL);
    mod2FreqParam.Init(*df.GetKnob(DaisyField::KNOB_5), 1, 6000, Parameter::EXPONENTIAL);
    index2Param.Init(*df.GetKnob(DaisyField::KNOB_6), 0, 10, Parameter::EXPONENTIAL);

    fold.Init();
    foldIncrParam.Init(*df.GetKnob(DaisyField::KNOB_7), 0, 100, Parameter::LINEAR);

    verb.Init(sample_rate);
    verb.SetFeedback(0.2f);
    verb.SetLpFreq(18000.0f);
    verbAmountParam.Init(*df.GetKnob(DaisyField::KNOB_8), 0, 1, Parameter::LINEAR);

    limiter.Init();

    df.StartAudio(AudioCallback);
    df.StartAdc();

    df.display.Fill(false);
    PrintOled(0, 0, "Dual FM");
    df.display.Update();
    PrintOled(0, 10, "free_art_ideas");
    df.display.Update();

    while(1)
    {
        df.ProcessAnalogControls();

        // if (tick.Process() != 0) {
        //   PrintSerial("K1: " + std::to_string(oscFreqParam.Process()));
        // }

        mod1.SetFreq(mod1FreqParam.Process());
        mod2.SetFreq(mod2FreqParam.Process());

        fold.SetIncrement(foldIncrParam.Process());

        verb.SetFeedback(verbAmountParam.Process());
    }
}

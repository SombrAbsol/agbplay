#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class CGBType : int { SQ1 = 0, SQ2, WAVE, NOISE };
enum class EnvState : int { INIT = 0, ATK, DEC, SUS, REL, PSEUDO_ECHO, DIE, DEAD };
enum class NoisePatt : int { FINE = 0, ROUGH };
enum class ReverbType { NORMAL, GS1, GS2, MGAT, TEST, NONE };
enum class ResamplerType { NEAREST, LINEAR, SINC, BLEP, BLAMP };
enum class CGBPolyphony { MONO_STRICT, MONO_SMOOTH, POLY };

ReverbType str2rev(const std::string& str);
std::string rev2str(ReverbType t);
ResamplerType str2res(const std::string& str);
std::string res2str(ResamplerType t);
CGBPolyphony str2cgbPoly(const std::string& str);
std::string cgbPoly2str(CGBPolyphony t);

struct MixingArgs
{
    float vol;
    uint32_t fixedModeRate;
    float sampleRateInv;
    float samplesPerBufferInv;
    size_t curInterFrame;   // <-- for debugging only
};

struct VolumeFade
{
    float fromVolLeft;
    float fromVolRight;
    float toVolLeft;
    float toVolRight;
};

struct ADSR
{
    uint8_t att = 0xFF;
    uint8_t dec = 0x00;
    uint8_t sus = 0xFF;
    uint8_t rel = 0x00;
};

struct Note
{
    uint8_t length;
    uint8_t midiKeyTrackData;
    uint8_t midiKeyPitch;
    uint8_t velocity;
    uint8_t priority;
    int8_t rhythmPan;
    uint8_t pseudoEchoVol;
    uint8_t pseudoEchoLen;
    uint8_t trackIdx;
    uint8_t playerIdx;
};

struct SampleInfo
{
    const int8_t *samplePtr;
    float midCfreq;
    uint32_t loopPos;
    uint32_t endPos;
    bool loopEnabled;
};

struct MP2KSoundMode
{
    const uint8_t vol;
    const uint8_t rev;
    const uint8_t freq;
    const uint8_t maxChannels = 0; // currently unused
    const uint8_t dacConfig = 0; // currently unused
};

struct AgbplaySoundMode
{
    // find a better name
    const ResamplerType resamplerTypeNormal;
    const ResamplerType resamplerTypeFixed;
    const ReverbType reverbType;
    const CGBPolyphony cgbPolyphony;
    const uint32_t dmaBufferLen;
    const uint8_t trackLimit;
    const int8_t maxLoops;
    const double padSilenceSecondsStart;
    const double padSilenceSecondsEnd;
    const bool accurateCh3Quantization;
    const bool accurateCh3Volume;
    const bool emulateCgbSustainBug; // other places may call this 'simulate', should probably use 'emulate' everywhere
};

struct SongTableInfo
{
    const size_t songTablePos;
    const size_t songCount;
};

struct SongInfo
{
    size_t songHeaderPos;
    size_t voiceTablePos;
    uint8_t reverb;
    uint8_t priority;
};

struct sample {
    float left;
    float right;
};

struct PlaylistEntry {
    std::string name;
    uint16_t id;
};

struct GameMatch {
    std::vector<std::string> gameCodes;
    std::vector<uint8_t> magicBytes;
};

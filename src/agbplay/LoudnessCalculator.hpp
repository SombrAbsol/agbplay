#pragma once

#include "Types.hpp"

#include <cstddef>
#include <span>

class LoudnessCalculator
{
public:
    LoudnessCalculator(const float lowpassFreq);
    LoudnessCalculator(const LoudnessCalculator &) = delete;
    LoudnessCalculator(LoudnessCalculator &&) = default;
    LoudnessCalculator &operator=(const LoudnessCalculator &) = delete;

    void CalcLoudness(std::span<const sample> buffer);
    void GetLoudness(float &rmsLeft, float &rmsRight, float &peakLeft, float &peakRight) const;
    void Reset();

private:
    static float calcAlpha(float lowpassFreq);

    float lpAlpha;
    float avgVolLeftSq = 0.0f;
    float avgVolRightSq = 0.0f;

    float rmsLeft = 0.0f;
    float rmsRight = 0.0f;
    float peakLeft = 0.0f;
    float peakRight = 0.0f;
};

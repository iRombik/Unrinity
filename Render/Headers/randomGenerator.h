#pragma once
#include <random>

float GetRandomValue(float min, float max)
{
    static std::random_device randDevice;
    static std::mt19937 gen(randDevice());
    const float d = max - min;
    return min + d * std::generate_canonical<float, 16>(gen);
}
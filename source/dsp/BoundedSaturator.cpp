#include "BoundedSaturator.h"

namespace braun::mr16 {

void BoundedSaturator::processBlock(const float* input, float* output, int numSamples) const noexcept {
    if (input == nullptr || output == nullptr || numSamples <= 0) {
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        output[i] = processSample(input[i]);
    }
}

void BoundedSaturator::processBlock(float* buffer, int numSamples) const noexcept {
    if (buffer == nullptr || numSamples <= 0) {
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = processSample(buffer[i]);
    }
}

} // namespace braun::mr16

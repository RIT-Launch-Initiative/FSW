#pragma once
#include "n_autocoder_types.h"

#include <cstddef>
namespace NBoost {

// note: barometer based boost detection is not implemented as it would incur too much of a delay for the airbrakes to do anything

void FeedDetector(float accelVertical);
bool IsDetected();

} // namespace NBoost

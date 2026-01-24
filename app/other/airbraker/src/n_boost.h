#include "n_autocoder_types.h"

namespace NBoost{
    void FeedAccel(const NTypes::AccelerometerData &data);
    bool IsDetected();
    // note: barometer based boost detection is not implemented as it would incur too much of a delay for the airbrakes to do anything 
}

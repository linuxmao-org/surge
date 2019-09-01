#pragma once
#include "AllLv2.h"

namespace SurgeLv2
{

const void *findFeature(const char *uri, const LV2_Feature *const *features);
const void *requireFeature(const char *uri, const LV2_Feature *const *features);

const LV2_Atom_Event *popNextEvent(
    const LV2_Atom_Sequence *sequence, const LV2_Atom_Event **iterator);
const LV2_Atom_Event *popNextEventAtFrame(
    const LV2_Atom_Sequence *sequence, const LV2_Atom_Event **iterator, uint32_t frameIndex);

struct DirectAccessInterface
{
    void *(*get_instance_pointer)(LV2_Handle handle);
};

} // namespace SurgeLv2

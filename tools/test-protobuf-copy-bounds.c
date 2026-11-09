/* Checks the pinned upb copy patch against valid and undersized objects. */
#include <stdint.h>
#include <string.h>
#include "upb/message/accessors.h"

int main (int argc, char** argv)
{
    uint8_t input[16], output[16];
    const unsigned reps[] = { kUpb_FieldRep_1Byte, kUpb_FieldRep_4Byte,
                              kUpb_FieldRep_8Byte, kUpb_FieldRep_StringView };
    const unsigned sizes[] = { 1, 4, 8, sizeof (upb_StringView) };

    for (int i = 0; i < 16; ++i)
        input[i] = (uint8_t) (i + 1);

    for (int i = 0; i < 4; ++i)
    {
        upb_MiniTableField field = { 0 };
        field.mode_dont_copy_me__upb_internal_use_only = (uint8_t) (reps[i] << kUpb_FieldRep_Shift);
        memset (output, 0, sizeof (output));
        _upb_MiniTableField_DataCopy_dont_copy_me__upb_internal_use_only (&field, output, input);

        if (memcmp (input, output, sizes[i]) != 0)
            return 1;
        for (unsigned j = sizes[i]; j < 16; ++j)
            if (output[j] != 0)
                return 2;
    }

#if defined(__GNUC__) && !defined(__clang__)
    if (argc > 1)
    {
        upb_MiniTableField field = { 0 };
        uint64_t tooSmall = 0;
        field.mode_dont_copy_me__upb_internal_use_only = kUpb_FieldRep_StringView << kUpb_FieldRep_Shift;
        if (strcmp (argv[1], "destination") == 0)
            _upb_MiniTableField_DataCopy_dont_copy_me__upb_internal_use_only (&field, &tooSmall, input);
        else
            _upb_MiniTableField_DataCopy_dont_copy_me__upb_internal_use_only (&field, output, &tooSmall);
        return 3;
    }
#else
    (void) argv;
    if (argc > 1)
        return 77;
#endif
    return 0;
}

#include "types.h"
#include <ver.h>

NO_DISCARD size_t get_type_size(TypeKind type) {
    switch (type) {
        case TYPE_PROCEDURE:
        case TYPE_LONG:
        case TYPE_POINTER: return 8;
        case TYPE_INT:     return 4;
        case TYPE_CHAR:    return 1;
        default: PANIC("invalid type");
    }
    UNREACHABLE();
}

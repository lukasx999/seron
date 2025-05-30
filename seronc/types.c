#include "types.h"
#include <ver.h>

NO_DISCARD int type_primitive_size(TypeKind type) {
    switch (type) {
        case TYPE_PROCEDURE:
        case TYPE_LONG:
        case TYPE_OBJECT:
        case TYPE_POINTER: return 8;
        case TYPE_INT:     return 4;
        case TYPE_CHAR:    return 1;
        case TYPE_INVALID:
        case TYPE_VOID:
        case TYPE_TABLE: PANIC("invalid type");
    }
    UNREACHABLE();
}

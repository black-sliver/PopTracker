#pragma once


enum class AccessibilityLevel : int {
    NONE = 0,
    PARTIAL = 1,
    INSPECT = 3,
    SEQUENCE_BREAK = 5,
    NORMAL = 6,
    CLEARED = 7,
};

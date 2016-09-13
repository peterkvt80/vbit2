/** ttxCodes
 * Character codes in teletext level 1
 * The comments show the wxTED keyboard shortcuts to get these codes
 */
#ifndef TTXCODES_H_INCLUDED
#define TTXCODES_H_INCLUDED


enum ttxCodes {
    ttxCodeAlphaBlack = 0, // <-- Not teletext level 1
    ttxCodeAlphaRed = 1,        // Shift-F1
    ttxCodeAlphaGreen = 2,      // Shift-F2
    ttxCodeAlphaYellow = 3,     // Shift-F3
    ttxCodeAlphaBlue = 4,       // Shift-F4
    ttxCodeAlphaMagenta = 5,    // Shift-F5
    ttxCodeAlphaCyan = 6,       // Shift-F6
    ttxCodeAlphaWhite = 7,      // Shift-F7
    ttxCodeFlash = 8,               // Ctrl-H
    ttxCodeSteady = 9,              // Ctrl-I
    ttxCodeEndBox = 10,             // Ctrl-J
    ttxCodeStartBox = 11,           // Ctrl-K
    ttxCodeNormalHeight = 12,       // Ctrl-L
    ttxCodeDoubleHeight = 13,       // Ctrl-M
    // double width etc goes here
    ttxCodeGraphicsBlack = 16,      // Level 2.5 and above
    ttxCodeGraphicsRed = 17,        // Ctrl-F1
    ttxCodeGraphicsGreen = 18,      // Ctrl-F2
    ttxCodeGraphicsYellow = 19,     // Ctrl-F3
    ttxCodeGraphicsBlue = 20,       // Ctrl-F4
    ttxCodeGraphicsMagenta = 21,    // Ctrl-F5
    ttxCodeGraphicsCyan = 22,       // Ctrl-F6
    ttxCodeGraphicsWhite = 23,      // Ctrl-F7
    ttxCodeConcealDisplay = 24,     // Ctrl-R
    ttxCodeContiguousGraphics = 25, // Ctrl-Y
    ttxCodeSeparatedGraphics = 26,  // Ctrl-T
    ttxCodeSwitch = 27,              // ESC Toggles between the first and second G0 sets defined by packets X/28/0 Format 1, X/28/4, M/29/0 or M/29/4.
    ttxCodeBlackBackground = 28,    // Ctrl-U
    ttxCodeNewBackground = 29,      // Ctrl-V
    ttxCodeHoldGraphics = 30,       // Ctrl-W
    ttxCodeReleaseGraphics = 31,    // Ctrl-X
};

/* Character mappings by locale
 * For an English keyboard with an English page...
 * Most keys are the same but but with a few exceptions:
 * <key> = <character>
 * [ = left arrow
 * \ = half
 * ] = right arrow
 * ^ = up arrow
 * ` = long dash
 * | = double pipe
 * } = three quarters
 * ~ = divide sign
 * { = quarter
 * ¬ = bullet block
 * \r = Double height mode
 */

#endif // TTXCODES_H_INCLUDED

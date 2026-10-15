#include "Kyoto/Graphics/CColor.hpp"

#ifdef TARGET_PC
// CColor(uint) stores the literal in host byte order; spell the channels so R/G/B/A land in
// the right bytes (Black() was R=255 A=0, leaving red under the multiply-blended cinema bars).
const CColor CColor::sBlackColor(uchar(0x00), uchar(0x00), uchar(0x00), uchar(0xFF));
const CColor CColor::sWhiteColor(uchar(0xFF), uchar(0xFF), uchar(0xFF), uchar(0xFF));
const CColor CColor::sGreyColor(uchar(0x80), uchar(0x80), uchar(0x80), uchar(0xFF));
const CColor CColor::sRedColor(uchar(0xFF), uchar(0x00), uchar(0x00), uchar(0xFF));
const CColor CColor::sGreenColor(uchar(0x00), uchar(0xFF), uchar(0x00), uchar(0xFF));
const CColor CColor::sBlueColor(uchar(0x00), uchar(0x00), uchar(0xFF), uchar(0xFF));
const CColor CColor::sYellowColor(uchar(0xFF), uchar(0xFF), uchar(0x00), uchar(0xFF));
const CColor CColor::sPurpleColor(uchar(0xA0), uchar(0x00), uchar(0xFF), uchar(0xFF));
const CColor CColor::sOrangeColor(uchar(0xFF), uchar(0x70), uchar(0x00), uchar(0xFF));
#else
const CColor CColor::sBlackColor(0x000000FF);
const CColor CColor::sWhiteColor(0xFFFFFFFF);
const CColor CColor::sGreyColor(0x808080FF);
const CColor CColor::sRedColor(0xFF0000FF);
const CColor CColor::sGreenColor(0x00FF00FF);
const CColor CColor::sBlueColor(0x0000FFFF);
const CColor CColor::sYellowColor(0xFFFF00FF);
const CColor CColor::sPurpleColor(0xA000FFFF);
const CColor CColor::sOrangeColor(0xFF7000FF);
#endif

const CColor& CColor::Black() { return sBlackColor; }
const CColor& CColor::White() { return sWhiteColor; }
const CColor& CColor::Grey() { return sGreyColor; }
const CColor& CColor::Red() { return sRedColor; }
const CColor& CColor::Green() { return sGreenColor; }
const CColor& CColor::Blue() { return sBlueColor; }
const CColor& CColor::Yellow() { return sYellowColor; }
const CColor& CColor::Purple() { return sPurpleColor; }
const CColor& CColor::Orange() { return sOrangeColor; }

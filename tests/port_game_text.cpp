// Standalone: c++ -std=c++17 -Wall -Wextra -Werror -Iplatform/include
// tests/port_game_text.cpp -o /tmp/opencode/port_game_text && /tmp/opencode/port_game_text
#include "port_discord.h"

#include "port_test.h"

int main() {
  using PortDiscord::GameTextToUtf8;
  REQUIRE(GameTextToUtf8(static_cast<const wchar_t*>(nullptr)).empty());
  REQUIRE(GameTextToUtf8(L"").empty());
  REQUIRE(GameTextToUtf8(L"Temple Grounds") == "Temple Grounds");
  REQUIRE(GameTextToUtf8(L"&line-spacing=75;\u0425\u0440\u0430\u043c\n\u00e9\r\u65e5") ==
         "\xd0\xa5\xd1\x80\xd0\xb0\xd0\xbc \xc3\xa9 \xe6\x97\xa5");
  REQUIRE(GameTextToUtf8(u"&just=center;A&&B &unfinished") == "A&B ");
  REQUIRE(GameTextToUtf8(u"\U0001f680") == "\xf0\x9f\x9a\x80");
  REQUIRE(GameTextToUtf8(U"\U0001f680") == "\xf0\x9f\x9a\x80");
  // STRG storage can retain UTF-16 pairs even when wchar_t itself is 32-bit.
  const wchar_t pair[] = {0xd83d, 0xde80, 0};
  REQUIRE(GameTextToUtf8(pair) == "\xf0\x9f\x9a\x80");
  const char16_t broken[] = {0xd83d, u'A', 0xde80, u'B', 0xd83d, 0};
  REQUIRE(GameTextToUtf8(broken) == "AB");
  const char32_t invalid[] = {0x110000, U'A', 0xffffffff, U'B', 0};
  REQUIRE(GameTextToUtf8(invalid) == "AB");
  REQUIRE(GameTextToUtf8(U"\U0010ffff") == "\xf4\x8f\xbf\xbf");
  const std::u16string longName(400, u'\u00e9');
  const std::string utf8 = GameTextToUtf8(longName.c_str());
  REQUIRE(utf8.size() == 800);
  for (size_t i = 0; i < utf8.size(); i += 2) {
    REQUIRE(utf8.substr(i, 2) == "\xc3\xa9");
  }
}

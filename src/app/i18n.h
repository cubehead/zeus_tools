#pragma once

#include "zeus/locale_preference.h"

#include <array>
#include <cstddef>

namespace app::i18n {

enum class Text {
    Auto, Theme, System, Light, Dark, Language, Chinese, English, Clear,
    PasteOrTypeJson, WaitingForInput, Processing, Invalid, Copied, Bytes,
    Format, Minify, Escape, Unescape, Table, DelimiterAuto, DelimiterComma,
    DelimiterSemicolon, DelimiterPipe, FirstRowHeader, On, Off, Decode, DecodeAgain,
    Inspect, Base64Encode, UrlEncode, Upper, Lower, HtmlEncode, HexEncode, UnicodeEncode, UnixTime,
    Digest, Hmac, HmacKey,
    MessageInput, MessageResult, KeyEncoding, ShowKey, HideKey,
    WeakAlgorithm, PreviewingFirst, Rows, RowsCopyTsv, TablePreviewCopyTsv,
    SearchResult, InvalidRegex, CopyAll, CopySelected, Selected, LocalOnly,
    About, Version, Build, AboutDescription, PrivacySummary, SpecialThanks,
    OpenSourceThanks, ProjectWebsite, License, OpenFile, ExportResult, Close,
    InputTooLarge, LargeInputPreview, ProcessAnyway, Count,
};

struct Translation {
    const char* zh_hans;
    const char* zh_hant;
    const char* en;
    const char* ja;
    const char* ko;
    const char* es;
    const char* fr;
    const char* de;
    const char* pt;
    const char* ru;
};

extern const std::array<Translation, static_cast<std::size_t>(Text::Count)> translations;
const char* get(Text text, zeus::LocalePreference locale);

} // namespace app::i18n

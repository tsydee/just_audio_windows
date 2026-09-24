#include "../uri_utils.hpp"

#include <gtest/gtest.h>

namespace just_audio_windows {
namespace test {

// ── EncodeSpacesInUri ────────────────────────────────────────────────────────

// A plain URL with no spaces should pass through unchanged.
TEST(EncodeSpacesInUri, NoSpaces_ReturnsUnchanged) {
  EXPECT_EQ(EncodeSpacesInUri("https://example.com/audio.mp3"),
            "https://example.com/audio.mp3");
}

// An empty string should stay empty.
TEST(EncodeSpacesInUri, EmptyString_ReturnsEmpty) {
  EXPECT_EQ(EncodeSpacesInUri(""), "");
}

// Literal spaces in a local file path must be encoded as %20.
// This is the core regression for https://github.com/bdlukaa/just_audio_windows/issues/26.
TEST(EncodeSpacesInUri, LiteralSpacesInFilePath_EncodedAsPercent20) {
  EXPECT_EQ(
      EncodeSpacesInUri("file:///C:/Users/My Files/song.mp3"),
      "file:///C:/Users/My%20Files/song.mp3");
}

// Multiple consecutive spaces must each be individually encoded.
TEST(EncodeSpacesInUri, MultipleSpaces_AllEncoded) {
  EXPECT_EQ(
      EncodeSpacesInUri(
          "C:/Users/HP/Downloads/Ve Kamleya Rocky Aur Rani.mp3"),
      "C:/Users/HP/Downloads/Ve%20Kamleya%20Rocky%20Aur%20Rani.mp3");
}

// Already percent-encoded spaces (%20) must NOT be double-encoded.
// This ensures a properly-encoded file:// URI like those produced by
// Dart's Uri.file() is left intact.
TEST(EncodeSpacesInUri, AlreadyEncodedSpaces_NotDoubleEncoded) {
  EXPECT_EQ(
      EncodeSpacesInUri("file:///C:/Users/My%20Files/song.mp3"),
      "file:///C:/Users/My%20Files/song.mp3");
}

// Percent-encoded multi-byte UTF-8 sequences (e.g. U+2019 RIGHT SINGLE
// QUOTATION MARK) must pass through unchanged so that Windows::Foundation::Uri
// can decode them natively.
// This is the core regression for the original apostrophe bug.
TEST(EncodeSpacesInUri, MultiBytePercentEncoded_Unchanged) {
  EXPECT_EQ(
      EncodeSpacesInUri(
          "https://example.com/speech?text=I%E2%80%99d%20like%20a%20coffee"),
      "https://example.com/speech?text=I%E2%80%99d%20like%20a%20coffee");
}

// A URL with both unencoded spaces and percent-encoded multi-byte chars:
// spaces must be encoded while the percent-sequences stay intact.
TEST(EncodeSpacesInUri, MixedLiteralSpacesAndPercentEncoded) {
  EXPECT_EQ(
      EncodeSpacesInUri(
          "https://example.com/speech?text=I%E2%80%99d like a coffee"),
      "https://example.com/speech?text=I%E2%80%99d%20like%20a%20coffee");
}

// A URL with only query-string spaces should have them encoded.
TEST(EncodeSpacesInUri, SpaceInQueryString) {
  EXPECT_EQ(
      EncodeSpacesInUri("https://example.com/tts?text=hello world"),
      "https://example.com/tts?text=hello%20world");
}

// ── FileUriToWindowsPath ─────────────────────────────────────────────────────

TEST(FileUriToWindowsPath, HttpsUri_IsNotAFile) {
  EXPECT_FALSE(FileUriToWindowsPath("https://example.com/audio.mp3").has_value());
}

TEST(FileUriToWindowsPath, DriveLetterPath_BackslashesAndNoLeadingSlash) {
  EXPECT_EQ(FileUriToWindowsPath("file:///C:/Users/HP/song.mp3"),
            "C:\\Users\\HP\\song.mp3");
}

TEST(FileUriToWindowsPath, PercentEncodedSpace_Decoded) {
  EXPECT_EQ(FileUriToWindowsPath("file:///C:/My%20Files/song.mp3"),
            "C:\\My Files\\song.mp3");
}

// The bug this helper exists for: Dart's File.uri percent-encodes a Cyrillic
// folder name as UTF-8, and Media Foundation cannot open that URL.
TEST(FileUriToWindowsPath, PercentEncodedUtf8_DecodedToUtf8Bytes) {
  EXPECT_EQ(
      FileUriToWindowsPath(
          "file:///C:/Users/rondi/OneDrive/"
          "%D0%94%D0%BE%D0%BA%D1%83%D0%BC%D0%B5%D0%BD%D1%82%D1%8B/devtest/a.m4a"),
      "C:\\Users\\rondi\\OneDrive\\"
      "\xD0\x94\xD0\xBE\xD0\xBA\xD1\x83\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x82\xD1\x8B"
      "\\devtest\\a.m4a");
}

TEST(FileUriToWindowsPath, LocalhostAuthority_TreatedAsLocal) {
  EXPECT_EQ(FileUriToWindowsPath("file://localhost/C:/a.mp3"), "C:\\a.mp3");
}

TEST(FileUriToWindowsPath, HostAuthority_BecomesUncPath) {
  EXPECT_EQ(FileUriToWindowsPath("file://server/share/a.mp3"),
            "\\\\server\\share\\a.mp3");
}

TEST(FileUriToWindowsPath, SchemeIsCaseInsensitive) {
  EXPECT_EQ(FileUriToWindowsPath("FILE:///C:/a.mp3"), "C:\\a.mp3");
}

TEST(FileUriToWindowsPath, QueryAndFragment_Dropped) {
  EXPECT_EQ(FileUriToWindowsPath("file:///C:/a.mp3?x=1#t"), "C:\\a.mp3");
}

// ── MimeTypeForPath ──────────────────────────────────────────────────────────

TEST(MimeTypeForPath, M4a_IsAudioMp4) {
  EXPECT_EQ(MimeTypeForPath("C:\\a\\b.m4a"), "audio/mp4");
}

TEST(MimeTypeForPath, ExtensionIsCaseInsensitive) {
  EXPECT_EQ(MimeTypeForPath("C:\\a\\B.MP3"), "audio/mpeg");
}

TEST(MimeTypeForPath, UnknownExtension_Empty) {
  EXPECT_EQ(MimeTypeForPath("C:\\a\\b.xyz"), "");
}

TEST(MimeTypeForPath, DotInDirectoryOnly_Empty) {
  EXPECT_EQ(MimeTypeForPath("C:\\a.b\\noext"), "");
}

}  // namespace test
}  // namespace just_audio_windows namespace just_audio_windows

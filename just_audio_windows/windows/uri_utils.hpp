#pragma once

#include <cctype>
#include <optional>
#include <string>

// Decodes "%XX" escapes to raw bytes; everything else passes through, and a
// malformed "%" is kept literally.
inline std::string PercentDecode(const std::string& s) {
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      const int hi = hex(s[i + 1]);
      const int lo = hex(s[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out += static_cast<char>(hi * 16 + lo);
        i += 2;
        continue;
      }
    }
    out += s[i];
  }
  return out;
}

// Converts a file: URI to a Windows path (UTF-8 in, UTF-8 out), or
// std::nullopt when |uri| is not a file: URI.
//
// Media Foundation resolves a file: URL with its own percent-decoding, which
// mangles multi-byte UTF-8 escapes: a file under C:\Users\Документы fails with
// ERROR_PATH_NOT_FOUND although it exists. Opening the file by path sidesteps
// that, so the URI is decoded here (RFC 3986 escapes are UTF-8) and the caller
// widens the result.
//   file:///C:/My%20Files/%D0%94/a.m4a -> C:\My Files\Д\a.m4a
//   file://localhost/C:/a.m4a          -> C:\a.m4a
//   file://server/share/a.m4a          -> \\server\share\a.m4a
inline std::optional<std::string> FileUriToWindowsPath(const std::string& uri) {
  if (uri.size() < 7) return std::nullopt;
  std::string scheme = uri.substr(0, 5);
  for (auto& c : scheme) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (scheme != "file:" || uri.compare(5, 2, "//") != 0) return std::nullopt;
  // A literal '?' or '#' in a file name is escaped by every URI producer, so an
  // unescaped one is a delimiter: drop query and fragment.
  const size_t end = uri.find_first_of("?#", 7);
  const std::string rest =
      uri.substr(7, end == std::string::npos ? std::string::npos : end - 7);
  const size_t slash = rest.find('/');
  const std::string host = rest.substr(0, slash);
  std::string path = slash == std::string::npos ? std::string() : rest.substr(slash);
  path = PercentDecode(path);
  for (auto& c : path) {
    if (c == '/') c = '\\';
  }
  if (host.empty() || host == "localhost") {
    // "\C:\..." -> "C:\..."
    if (path.size() >= 3 && path[0] == '\\' && path[2] == ':') path.erase(0, 1);
    return path;
  }
  return "\\\\" + host + path;
}

// The MIME type Media Foundation should assume for a local file, by extension.
// Empty when unknown, which lets it sniff the bytes instead.
inline std::string MimeTypeForPath(const std::string& path) {
  const size_t dot = path.find_last_of('.');
  const size_t sep = path.find_last_of("\\/");
  if (dot == std::string::npos || (sep != std::string::npos && sep > dot)) return "";
  std::string ext = path.substr(dot + 1);
  for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (ext == "m4a" || ext == "mp4" || ext == "m4b") return "audio/mp4";
  if (ext == "mp3") return "audio/mpeg";
  if (ext == "wav") return "audio/wav";
  if (ext == "aac") return "audio/aac";
  if (ext == "flac") return "audio/flac";
  if (ext == "ogg" || ext == "oga" || ext == "opus") return "audio/ogg";
  if (ext == "wma") return "audio/x-ms-wma";
  if (ext == "webm") return "audio/webm";
  if (ext == "mkv" || ext == "mka") return "audio/x-matroska";
  return "";
}

// Encodes literal space characters in a URI string as "%20" so that
// Windows::Foundation::Uri accepts URIs that contain unencoded spaces
// (e.g. local file paths like "file:///C:/My Files/song.mp3").
//
// Already percent-encoded sequences such as "%20" or "%E2%80%99" are never
// modified, because they contain no literal space character.
inline std::string EncodeSpacesInUri(const std::string& uri) {
  std::string encoded;
  // Reserve worst-case capacity (every char is a space → 3 chars each).
  encoded.reserve(uri.length() * 3);
  for (char c : uri) {
    if (c == ' ') {
      encoded += "%20";
    } else {
      encoded += c;
    }
  }
  return encoded;
}

//////////
#include "pe.hpp"
#include <filesystem>
#include <unordered_set>
#include <clocale>

using dllset_t = std::unordered_set<std::wstring>;

class DepthAuxiliary {
public:
  DepthAuxiliary(int &i) : depth(i) { depth++; }
  ~DepthAuxiliary() { depth--; }

private:
  int &depth;
};

class NoDependsEngine {
public:
  NoDependsEngine() = default;
  bool Initialize(std::wstring_view file) {
    std::error_code ec;
    exefile = std::filesystem::canonical(file, ec);
    if (ec) {
      fwprintf(stderr, L"File: '%.*s' not found\n", (int)file.size(),
               file.data());
      return false;
    }
    dir = exefile.parent_path();
    return true;
  }
  bool Parse() { return ParsePE(exefile.wstring()); }
  const dllset_t &Dlls() const { return ds; }
  bool CopyToDist(std::wstring_view dist);

private:
  std::filesystem::path exefile;
  std::filesystem::path dir;
  dllset_t ds;
  bool ParsePE(std::wstring_view file);
  bool IsDllExistsUnrecorded(const std::wstring &dll, std::wstring &dp);
  int depth{0};
};

bool NoDependsEngine::IsDllExistsUnrecorded(const std::wstring &dll,
                                            std::wstring &dp) {
  auto it = ds.find(dll);
  if (it != ds.end()) {
    /// is record
    return false;
  }
  std::error_code ec;
  auto dllpath = dir / dll;
  if (std::filesystem::exists(dllpath, ec)) {
    dp = dllpath.wstring();
    return true;
  }
  return false;
}

bool NoDependsEngine::ParsePE(std::wstring_view pefile) {
  base::error_code ec;
  auto pm = pecoff::inquisitive_pecoff(pefile, ec);
  DepthAuxiliary da(depth);
  if (depth >= 2000) {
    return false;
  }
  if (ec) {
    return false;
  }
  for (const auto &e : pm->depends) {
    std::wstring dll;
    if (IsDllExistsUnrecorded(e, dll)) {
      if (!ParsePE(dll)) {
        return false;
      }
      ds.insert(dll);
    }
  }
  return true;
}

class CopyAuxiliary {
public:
  CopyAuxiliary() = default;
  ~CopyAuxiliary() {
    if (!p.empty()) {
      std::error_code ec;
      /// remove
      std::filesystem::remove(p, ec);
      if (ec) {
        fprintf(stderr, "remove %s\n", ec.message().c_str());
      }
    }
  }
  bool Temp(const std::filesystem::path &file) {
    std::error_code ec;
    if (std::filesystem::exists(file, ec)) {
      p = file;
      p += L".tmp";
      std::filesystem::rename(file, p, ec);
      if (ec) {
        p.clear();
        return false;
      }
    }
    return true;
  }
  void Restore() {
    if (p.empty()) {
      return;
    }
    auto w = p.wstring();
    std::filesystem::path rp(w.substr(0, w.size() - 4));
    std::error_code ec;
    std::filesystem::rename(p, rp, ec);
    if (!ec) {
      p.clear();
    }
  }

private:
  std::filesystem::path p;
};

bool FileCopyTo(const std::filesystem::path &file,
                const std::filesystem::path &dir) {
  auto target = dir / file.filename();
  CopyAuxiliary ca;
  if (!ca.Temp(target)) {
    return false;
  }
  std::error_code ec;
  if (!std::filesystem::copy_file(file, target, ec)) {
    ca.Restore();
    return false;
  }
  return true;
}

bool NoDependsEngine::CopyToDist(std::wstring_view dist) {
  std::error_code ec;
  auto distdir = std::filesystem::absolute(dist, ec);
  if (ec) {
    fprintf(stderr, "Absolute %s\n", ec.message().c_str());
    return false;
  }
  if (!std::filesystem::exists(distdir, ec) &&
      !std::filesystem::create_directories(distdir, ec)) {
    fwprintf(stderr, L"Unable mkdirall '%s'\n", distdir.c_str());
    return false;
  }
  if (!FileCopyTo(exefile, distdir)) {
    fwprintf(stderr, L"Unable copy '%s' to '%s'\n", exefile.c_str(),
             distdir.c_str());
    return false;
  }
  for (const auto &d : ds) {
    if (!FileCopyTo(d, distdir)) {
      fwprintf(stderr, L"Unable copy '%s' to '%s'\n", exefile.c_str(),
               distdir.c_str());
      return false;
    }
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  _wsetlocale(LC_ALL, nullptr);
  if (argc < 2) {
    wprintf(L"usage: %s exefile dist\n", argv[0]);
    return 1;
  }
  NoDependsEngine nd;
  if (!nd.Initialize(argv[1])) {
    return 1;
  }
  if (!nd.Parse()) {
    return 1;
  }
  wprintf(L"Exe: %s depends:\n", argv[1]);
  for (const auto &e : nd.Dlls()) {
    wprintf(L"     %s\n", e.c_str());
  }
  if (argc < 3) {
    return 1;
  }
  if (!nd.CopyToDist(argv[2])) {
    fwprintf(stderr, L"Copy to '%s' failure\n", argv[2]);
    return 1;
  }
  fwprintf(stderr, L"Copy to '%s' success\n", argv[2]);
  return 0;
}
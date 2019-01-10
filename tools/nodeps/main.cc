//////////
#include "pe.hpp"
#include <filesystem>
#include <unordered_set>

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
    if (ec || !std::filesystem::exists(exefile, ec)) {
      fwprintf(stderr, L"File: '%.*s' not found\n", (int)file.size(),
               file.data());
      return false;
    }
    dir = exefile.parent_path();
    return true;
  }
  bool Parse() { return ParsePE(exefile.wstring()); }
  const dllset_t &Dlls() const { return ds; }
  bool CopyToDist();

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

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    wprintf(L"usage: %s pe\n", argv[0]);
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
  return 0;
}
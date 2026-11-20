#define DONT_SET_USING_JUCE_NAMESPACE 1
#include "Updater.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <shellapi.h>
#endif

namespace updater
{
//==============================================================================
static juce::Array<int> parseVersion (const juce::String& v)
{
    const auto core = v.trim().trimCharactersAtStart ("vV").upToFirstOccurrenceOf ("-", false, false);
    juce::Array<int> r;
    for (auto& part : juce::StringArray::fromTokens (core, ".", ""))
        r.add (part.getIntValue());
    while (r.size() < 3) r.add (0);
    return r;
}

bool isNewer (const juce::String& a, const juce::String& b)
{
    const auto x = parseVersion (a), y = parseVersion (b);
    for (int i = 0; i < juce::jmax (x.size(), y.size()); ++i)
    {
        const int xi = i < x.size() ? x[i] : 0, yi = i < y.size() ? y[i] : 0;
        if (xi != yi) return xi > yi;
    }
    return false;
}

static juce::String userAgent() { return "User-Agent: BerryTone-Updater/" JucePlugin_VersionString "\r\n"; }

//==============================================================================
UpdateChecker::UpdateChecker() : juce::Thread ("BerryTone update check") {}
UpdateChecker::~UpdateChecker() { stopThread (4000); }

void UpdateChecker::checkOnce()
{
    if (! started.exchange (true))
        startThread();
}

ReleaseInfo UpdateChecker::getInfo() const
{
    const juce::ScopedLock sl (lock);
    return info;
}

void UpdateChecker::run()
{
    juce::URL url ("https://api.github.com/repos/" + juce::String (githubRepo) + "/releases/latest");
    int statusCode = 0;
    auto stream = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                             .withExtraHeaders (userAgent() + "Accept: application/vnd.github+json\r\n")
                                             .withConnectionTimeoutMs (8000)
                                             .withNumRedirectsToFollow (5)
                                             .withStatusCode (&statusCode));
    if (stream == nullptr || threadShouldExit())
        return;

    const auto json = juce::JSON::parse (stream->readEntireStreamAsString());
    if (statusCode != 200 || ! json.isObject())
        return;

    ReleaseInfo r;
    r.version = json["tag_name"].toString().trimCharactersAtStart ("vV");
    r.notes   = json["body"].toString().trim();
    r.pageUrl = json["html_url"].toString();
    if (auto* assets = json["assets"].getArray())
        for (auto& a : *assets)
            if (a["name"].toString().endsWithIgnoreCase ("-win64.zip"))
                r.assetUrl = a["browser_download_url"].toString();

    if (r.version.isEmpty() || ! isNewer (r.version, JucePlugin_VersionString))
        return;

    {
        const juce::ScopedLock sl (lock);
        info = r;
    }
    found = true;

    juce::MessageManager::callAsync ([safe = juce::WeakReference<UpdateChecker> (this)]
    {
        if (safe != nullptr && safe->onUpdateFound)
            safe->onUpdateFound();
    });
}

//==============================================================================
UpdateInstaller::UpdateInstaller() : juce::Thread ("BerryTone update install") {}
UpdateInstaller::~UpdateInstaller() { stopThread (10000); }

void UpdateInstaller::start (const ReleaseInfo& r)
{
    if (isThreadRunning()) return;
    rel = r;
    progress = 0.0;
    startThread();
}

void UpdateInstaller::finish (bool ok, const juce::String& msg)
{
    juce::MessageManager::callAsync ([safe = juce::WeakReference<UpdateInstaller> (this), ok, msg]
    {
        if (safe != nullptr && safe->onFinished)
            safe->onFinished (ok, msg);
    });
}

bool UpdateInstaller::download (juce::File& zipOut, juce::String& err)
{
    juce::URL url (rel.assetUrl);
    int statusCode = 0;
    auto in = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                         .withExtraHeaders (userAgent() + "Accept: application/octet-stream\r\n")
                                         .withConnectionTimeoutMs (15000)
                                         .withNumRedirectsToFollow (10)
                                         .withStatusCode (&statusCode));
    if (in == nullptr || (statusCode != 0 && statusCode != 200))
    {
        err = "Cannot download the update (HTTP " + juce::String (statusCode) + ").";
        return false;
    }

    zipOut.deleteFile();
    juce::FileOutputStream out (zipOut);
    if (! out.openedOk())
    {
        err = "Cannot write " + zipOut.getFullPathName();
        return false;
    }

    const auto total = in->getTotalLength();
    juce::int64 done = 0;
    juce::HeapBlock<char> buf (65536);
    while (! in->isExhausted())
    {
        if (threadShouldExit()) { err = "Canceled."; return false; }
        const int n = in->read (buf.get(), 65536);
        if (n <= 0) break;
        out.write (buf.get(), (size_t) n);
        done += n;
        progress = total > 0 ? (double) done / (double) total : -1.0;
    }
    out.flush();
    if (done < 1024)
    {
        err = "The downloaded file is empty.";
        return false;
    }
    return true;
}

static bool canWriteTo (juce::File dir)
{
    while (! dir.isDirectory() && dir.getParentDirectory() != dir)
        dir = dir.getParentDirectory();
    auto test = dir.getChildFile (".berrytone_write_test");
    const bool ok = test.replaceWithText ("x");
    test.deleteFile();
    return ok;
}

static juce::String psQuote (const juce::String& s) { return "'" + s.replace ("'", "''") + "'"; }

juce::String makeInstallScript (const juce::File& src, const juce::File& dst, const juce::File& log)
{
    juce::String ps;
    ps << "$ErrorActionPreference = 'Stop'\r\n"
       << "$log = " << psQuote (log.getFullPathName()) << "\r\n"
       << "function Move-Old([string]$f) {\r\n"
       << "  if (Test-Path -LiteralPath $f) {\r\n"
       << "    $dir = [System.IO.Path]::GetDirectoryName($f)\r\n"
       << "    $leaf = [System.IO.Path]::GetFileName($f)\r\n"
       << "    Get-ChildItem -LiteralPath $dir -Filter ($leaf + '.old*') -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue\r\n"
       << "    Rename-Item -LiteralPath $f -NewName ($leaf + '.old' + (Get-Date -Format 'yyyyMMddHHmmss'))\r\n"
       << "  }\r\n"
       << "}\r\n"
       << "try {\r\n"
       << "  $src = " << psQuote (src.getFullPathName()) << "\r\n"
       << "  $dst = " << psQuote (dst.getFullPathName()) << "\r\n"
       << "  Move-Old $dst\r\n"     // an old bundle folder at the target path is renamed too
       << "  New-Item -ItemType Directory -Force -Path ([System.IO.Path]::GetDirectoryName($dst)) | Out-Null\r\n"
       << "  Copy-Item -LiteralPath $src -Destination $dst -Force\r\n"
       << "  'OK' | Out-File -LiteralPath $log -Encoding utf8\r\n"
       << "  exit 0\r\n"
       << "} catch {\r\n"
       << "  ('ERROR: ' + $_.Exception.Message) | Out-File -LiteralPath $log -Encoding utf8\r\n"
       << "  exit 1\r\n"
       << "}\r\n";
    return ps;
}

bool UpdateInstaller::runInstallScript (const juce::File& script, bool elevated, juce::String& err)
{
   #if JUCE_WINDOWS
    const juce::String params = "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"" + script.getFullPathName() + "\"";

    SHELLEXECUTEINFOW sei {};
    sei.cbSize = sizeof (sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = elevated ? L"runas" : L"open";
    sei.lpFile = L"powershell.exe";
    sei.lpParameters = params.toWideCharPointer();
    sei.nShow = SW_HIDE;

    if (! ShellExecuteExW (&sei) || sei.hProcess == nullptr)
    {
        err = GetLastError() == ERROR_CANCELLED ? "The install was canceled (no admin rights)."
                                                : "Cannot start the install script.";
        return false;
    }

    for (;;)
    {
        if (WaitForSingleObject (sei.hProcess, 250) == WAIT_OBJECT_0) break;
        if (threadShouldExit()) { CloseHandle (sei.hProcess); err = "Canceled."; return false; }
    }
    DWORD code = 1;
    GetExitCodeProcess (sei.hProcess, &code);
    CloseHandle (sei.hProcess);
    return code == 0;
   #else
    juce::ignoreUnused (script, elevated);
    err = "Automatic install works on Windows only.";
    return false;
   #endif
}

void UpdateInstaller::run()
{
   #if ! JUCE_WINDOWS
    finish (false, "Automatic install works on Windows only. Download the update from the release page.");
    return;
   #else
    if (rel.assetUrl.isEmpty())
        return finish (false, "This release has no Windows file. Open the release page.");

    auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BerryTone-update");
    root.deleteRecursively();
    if (! root.createDirectory())
        return finish (false, "Cannot create the folder " + root.getFullPathName());

    // 1. download
    auto zipFile = root.getChildFile ("BerryTone-v" + rel.version + "-win64.zip");
    juce::String err;
    if (! download (zipFile, err))
        return finish (false, err);

    // 2. unpack
    progress = -1.0;
    auto unpacked = root.getChildFile ("files");
    {
        juce::ZipFile zip (zipFile);
        if (zip.getNumEntries() == 0 || zip.uncompressTo (unpacked, true).failed())
            return finish (false, "Cannot unpack the update.");
    }
    // The release zip holds BerryTone.vst3 as one file (the plugin DLL).
    juce::File newDll;
    for (auto& f : unpacked.findChildFiles (juce::File::findFiles, true, "BerryTone.vst3"))
        { newDll = f; break; }
    if (! newDll.existsAsFile())
        return finish (false, "The update has no BerryTone.vst3.");

    // 3. find the installed file.
    //    - Loaded from a single file: replace that file.
    //    - Loaded from an old bundle folder: replace the DLL inside the bundle
    //      (the DAW already knows this path).
    //    - Otherwise: %CommonProgramFiles%\VST3\BerryTone.vst3.
    const auto common = juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files");
    auto dst = juce::File (common).getChildFile ("VST3").getChildFile ("BerryTone.vst3");
    const auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_VST3
         && self.hasFileExtension (".vst3") && self.existsAsFile())
        dst = self;

    // 4. write and run the install script. A loaded DLL can be renamed but
    //    not overwritten, so the script renames the old file first.
    auto log = root.getChildFile ("install.log");
    const auto ps = makeInstallScript (newDll, dst, log);

    auto script = root.getChildFile ("install.ps1");
    if (! script.replaceWithText (ps, true, true))   // UTF-16 with BOM: safe for any path
        return finish (false, "Cannot write the install script.");

    const bool elevated = ! canWriteTo (dst.getParentDirectory());
    if (! runInstallScript (script, elevated, err))
    {
        if (err.isEmpty())
            err = log.existsAsFile() ? log.loadFileAsString().trim() : juce::String ("The install script failed.");
        return finish (false, err);
    }

    progress = 1.0;
    finish (true, "BerryTone v" + rel.version + " is installed in:\n" + dst.getFullPathName()
                      + "\n\nRestart the DAW to use the new version.");
   #endif
}
} // namespace updater

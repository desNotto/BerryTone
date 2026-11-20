#include "PresetManager.h"
#include "PluginProcessor.h"

namespace
{
    struct FactoryPreset
    {
        const char* name;
        Design design;
        std::initializer_list<std::pair<const char*, float>> values;
    };

    // A factory preset first resets every parameter to its default, then sets its values.
    // Preset 1 "Init" is the first-launch preset: all modules off, the knobs at their defaults.
    const FactoryPreset factory[] =
    {
        { "Init", Design::berry,
          {
              { ID::tightOn, 0.f }, { ID::gateOn, 0.f }, { ID::chugOn, 0.f }, { ID::octOn, 0.f },
              { ID::impOn, 0.f },   { ID::tiltOn, 0.f }, { ID::destOn, 0.f },
              { ID::inGain, 0.f },  { ID::outGain, 0.f }
          } },
        // From the user preset file "9 string crunch.btpreset" (v1.1.1).
        { "9 string crunch", Design::grape,
          {
              { ID::inGain, 0.f },          { ID::outGain, 0.f },
              { ID::tightOn, 1.f },         { ID::tightFreq, 99.52f },  { ID::tightAmt, 1.f },
              { ID::gateOn, 0.f },          { ID::gateThresh, -55.f },  { ID::gateRel, 80.f },   { ID::gateDepth, -70.f },
              { ID::chugOn, 0.f },          { ID::chugPunch, 0.3f },    { ID::chugBody, 0.f },   { ID::chugSpeed, 2.f },
              { ID::octOn, 0.f },           { ID::octBlend, 0.884f },   { ID::octTone, 109.71f }, { ID::octTrack, 498.93f },
              { ID::impOn, 1.f },           { ID::impFreq, 4600.94f },  { ID::impPeak, 0.748f }, { ID::impLoad, 0.4f },
              { ID::tiltOn, 1.f },          { ID::tiltAmt, 8.f },       { ID::tiltPivot, 600.f },
              { ID::destOn, 0.f },          { ID::destDrive, 0.916f },  { ID::destCrush, 0.228f }, { ID::destMix, 0.948f }
          } }
    };
    constexpr int numFactory = (int) (sizeof (factory) / sizeof (factory[0]));
}

PresetManager::PresetManager (BerryToneAudioProcessor& p) : proc (p)
{
    refresh();
}

juce::File PresetManager::getUserDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("desNotto").getChildFile ("BerryTone").getChildFile ("Presets");
}

juce::String PresetManager::cleanName (const juce::String& n)
{
    return juce::File::createLegalFileName (n.trim()).trim();
}

juce::File PresetManager::fileFor (const juce::String& name) const
{
    return getUserDir().getChildFile (cleanName (name) + fileExt);
}

void PresetManager::refresh()
{
    entries.clear();
    for (int i = 0; i < numFactory; ++i)
        entries.push_back ({ factory[i].name, true, i, {}, factory[i].design });

    std::vector<Entry> user;
    for (const auto& f : getUserDir().findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExt))
    {
        Entry e { f.getFileNameWithoutExtension(), false, -1, f, Design::berry };
        bool sameAsFactory = false;   // a user file with a factory name: the factory preset wins
        for (int i = 0; i < numFactory; ++i)
            sameAsFactory = sameAsFactory || e.name.equalsIgnoreCase (factory[i].name);
        if (sameAsFactory) continue;
        if (auto xml = juce::XmlDocument::parse (f))
            e.design = xml->getStringAttribute ("design") == "grape" ? Design::grape : Design::berry;
        user.push_back (e);
    }
    std::sort (user.begin(), user.end(), [] (const Entry& a, const Entry& b)
               { return a.name.compareNatural (b.name) < 0; });
    entries.insert (entries.end(), user.begin(), user.end());
}

int PresetManager::getCurrentIndex() const
{
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].factory == curFactory && entries[i].name == curName)
            return (int) i;
    return -1;
}

juce::String PresetManager::getCurrentName() const
{
    return curName.isEmpty() ? juce::String ("(unsaved)") : curName;
}

std::vector<float> PresetManager::snapshot() const
{
    std::vector<float> v;
    for (auto* prm : proc.getParameters()) v.push_back (prm->getValue());
    for (int o : proc.getOrder()) v.push_back ((float) o);
    v.push_back ((float) proc.getDesign());
    return v;
}

bool PresetManager::isModified() const
{
    return curName.isNotEmpty() && snapshot() != loadedSnapshot;
}

void PresetManager::setCurrent (const juce::String& name, bool isFactory)
{
    curName = name;
    curFactory = isFactory;
    proc.apvts.state.setProperty ("presetName", name, nullptr);
    proc.apvts.state.setProperty ("presetFactory", isFactory, nullptr);
    loadedSnapshot = snapshot();
}

void PresetManager::applyFactory (int index)
{
    const auto& fp = factory[index];
    for (auto* prm : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& kv : fp.values)
        if (auto* prm = proc.apvts.getParameter (kv.first))
            prm->setValueNotifyingHost (prm->convertTo0to1 (kv.second));

    proc.setOrder ({ 0, 1, 2, 3, 4, 5, 6 });
    proc.setDesign (fp.design);
}

bool PresetManager::loadIndex (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) entries.size()))
        return false;

    const auto e = entries[(size_t) index];
    if (e.factory)
    {
        applyFactory (e.factoryIndex);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (e.file);
        if (xml == nullptr || ! proc.stateFromXml (*xml))
            return false;
    }
    setCurrent (e.name, e.factory);
    return true;
}

void PresetManager::next()
{
    refresh();
    if (entries.empty()) return;
    loadIndex ((getCurrentIndex() + 1) % (int) entries.size());
}

void PresetManager::previous()
{
    refresh();
    if (entries.empty()) return;
    const int i = getCurrentIndex();
    loadIndex (i <= 0 ? (int) entries.size() - 1 : i - 1);
}

juce::Result PresetManager::writeFile (const juce::File& f)
{
    auto xml = proc.stateToXml();
    if (xml == nullptr)
        return juce::Result::fail ("Cannot read the plugin state.");

    xml->removeAttribute ("presetName");
    xml->removeAttribute ("presetFactory");
    xml->setAttribute ("presetFormat", 1);
    xml->setAttribute ("pluginVersion", JucePlugin_VersionString);

    if (! f.getParentDirectory().createDirectory())
        return juce::Result::fail ("Cannot create the folder " + f.getParentDirectory().getFullPathName());
    if (! xml->writeTo (f))
        return juce::Result::fail ("Cannot write the file " + f.getFullPathName());
    return juce::Result::ok();
}

bool PresetManager::userPresetExists (const juce::String& name) const
{
    return fileFor (name).existsAsFile();
}

juce::Result PresetManager::save()
{
    if (curName.isEmpty() || curFactory)
        return juce::Result::fail ("Factory presets are read only. Use Save as.");

    auto r = writeFile (fileFor (curName));
    if (r.wasOk()) { refresh(); setCurrent (curName, false); }
    return r;
}

juce::Result PresetManager::saveAs (const juce::String& name)
{
    const auto n = cleanName (name);
    if (n.isEmpty())
        return juce::Result::fail ("Type a name for the preset.");
    for (int i = 0; i < numFactory; ++i)
        if (n.equalsIgnoreCase (factory[i].name))
            return juce::Result::fail ("This name belongs to a factory preset. Type a different name.");

    auto r = writeFile (fileFor (n));
    if (r.wasOk()) { refresh(); setCurrent (n, false); }
    return r;
}

juce::Result PresetManager::rename (const juce::String& newName)
{
    if (curName.isEmpty() || curFactory)
        return juce::Result::fail ("You cannot rename a factory preset.");

    const auto n = cleanName (newName);
    if (n.isEmpty())
        return juce::Result::fail ("Type a name for the preset.");
    if (n == curName)
        return juce::Result::ok();
    for (int i = 0; i < numFactory; ++i)
        if (n.equalsIgnoreCase (factory[i].name))
            return juce::Result::fail ("This name belongs to a factory preset. Type a different name.");

    const auto target = fileFor (n);
    if (target.existsAsFile() && ! n.equalsIgnoreCase (curName))
        return juce::Result::fail ("A preset with this name exists. Type a different name.");
    if (! fileFor (curName).moveFileTo (target))
        return juce::Result::fail ("Cannot rename the file.");

    const auto snap = loadedSnapshot;
    refresh();
    curName = n;
    proc.apvts.state.setProperty ("presetName", n, nullptr);
    loadedSnapshot = snap;   // a rename does not change the "modified" state
    return juce::Result::ok();
}

juce::Result PresetManager::remove()
{
    if (curName.isEmpty() || curFactory)
        return juce::Result::fail ("You cannot delete a factory preset.");
    if (! fileFor (curName).deleteFile())
        return juce::Result::fail ("Cannot delete the file.");

    refresh();
    // The sound stays as it is. It is now an unsaved state.
    curName = {};
    curFactory = false;
    proc.apvts.state.setProperty ("presetName", "", nullptr);
    proc.apvts.state.setProperty ("presetFactory", false, nullptr);
    return juce::Result::ok();
}

void PresetManager::syncFromState()
{
    refresh();
    curName = proc.apvts.state.getProperty ("presetName", "").toString();
    curFactory = (bool) proc.apvts.state.getProperty ("presetFactory", false);

    // a session from an older version may name a factory preset that is gone (e.g. "Berry Base")
    if (curFactory)
    {
        bool exists = false;
        for (int i = 0; i < numFactory; ++i)
            exists = exists || curName == factory[i].name;
        if (! exists) { curName = {}; curFactory = false; }
    }
    loadedSnapshot = snapshot();
}

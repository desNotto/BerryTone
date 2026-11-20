// Dev tool: draws the plugin window into PNG files, for layout checks
// without a DAW. Not part of the release. Build: -DBERRY_SNAPSHOT=ON
//   BerryToneSnapshot <outDir>
#include "../../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    for (auto d : { Design::berry, Design::grape })
        for (float s : { 1.0f, 1.25f, 0.8f })
        {
            BerryToneAudioProcessor proc;
            proc.setDesign (d);
            proc.uiScale = s;
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            const auto name = juce::String (d == Design::grape ? "grape" : "berry") + "_" + juce::String (juce::roundToInt (s * 100)) + ".png";
            juce::FileOutputStream out (outDir.getChildFile (name));
            out.setPosition (0); out.truncate();
            juce::PNGImageFormat().writeImageToStream (img, out);
            std::printf ("%s %dx%d\n", name.toRawUTF8(), ed->getWidth(), ed->getHeight());
            ed.reset();
        }
    return 0;
}

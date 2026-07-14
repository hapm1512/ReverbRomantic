#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr std::array<const char*, 16> presetParameterIds {
    Parameters::IDs::mix, Parameters::IDs::decay, Parameters::IDs::time, Parameters::IDs::preDelay,
    Parameters::IDs::size, Parameters::IDs::width, Parameters::IDs::warmth, Parameters::IDs::brightness,
    Parameters::IDs::diffusion, Parameters::IDs::density, Parameters::IDs::modulation, Parameters::IDs::bloom,
    Parameters::IDs::ducking, Parameters::IDs::lowCut, Parameters::IDs::highCut, Parameters::IDs::output
};
}

ReverbRomanticAudioProcessor::ReverbRomanticAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", Parameters::createParameterLayout())
{
    snapshotA = apvts.copyState();
    snapshotB = snapshotA.createCopy();
    loadPersistentMetadata();
}

const std::array<ReverbRomanticAudioProcessor::FactoryPreset, 48>&
ReverbRomanticAudioProcessor::getFactoryPresetTable() noexcept
{
    static const std::array<FactoryPreset, 48> presets {{
        { "Velvet Vocal", "Romantic Vocal", Algorithm::romanticHall, { 34.000f, 4.200f, 1.000f, 38.000f, 110.000f, 128.000f, 2.600f, -1.300f, 89.000f, 94.000f, 22.000f, 46.000f, 20.000f, 120.000f, 12000.000f, 0.000f } },
        { "Silky Lead", "Romantic Vocal", Algorithm::romanticHall, { 35.500f, 4.550f, 1.025f, 41.000f, 114.000f, 133.000f, 3.000f, -1.000f, 90.200f, 94.800f, 23.500f, 48.500f, 21.000f, 115.000f, 12450.000f, -0.250f } },
        { "Lyrical Voice", "Romantic Vocal", Algorithm::romanticHall, { 37.000f, 4.900f, 1.050f, 44.000f, 118.000f, 138.000f, 3.400f, -0.700f, 91.400f, 95.600f, 25.000f, 51.000f, 22.000f, 110.000f, 12900.000f, -0.500f } },
        { "Romantic Air", "Romantic Vocal", Algorithm::romanticHall, { 38.500f, 5.250f, 1.075f, 47.000f, 122.000f, 143.000f, 3.800f, -0.400f, 92.600f, 96.400f, 26.500f, 53.500f, 23.000f, 105.000f, 13350.000f, -0.750f } },
        { "Warm Ballad", "Ballad", Algorithm::romanticHall, { 31.000f, 3.800f, 0.960f, 30.000f, 104.000f, 120.000f, 4.600f, -2.300f, 85.000f, 91.000f, 16.000f, 38.000f, 16.000f, 135.000f, 10800.000f, 0.000f } },
        { "Soft Ballad", "Ballad", Algorithm::romanticHall, { 32.500f, 4.150f, 0.985f, 33.000f, 108.000f, 125.000f, 5.000f, -2.000f, 86.200f, 91.800f, 17.500f, 40.500f, 17.000f, 130.000f, 11250.000f, -0.250f } },
        { "Wide Ballad", "Ballad", Algorithm::romanticHall, { 34.000f, 4.500f, 1.010f, 36.000f, 112.000f, 130.000f, 5.400f, -1.700f, 87.400f, 92.600f, 19.000f, 43.000f, 18.000f, 125.000f, 11700.000f, -0.500f } },
        { "Deep Ballad", "Ballad", Algorithm::romanticHall, { 35.500f, 4.850f, 1.035f, 39.000f, 116.000f, 135.000f, 5.800f, -1.400f, 88.600f, 93.400f, 20.500f, 45.500f, 19.000f, 120.000f, 12150.000f, -0.750f } },
        { "Bolero Classic", "Bolero", Algorithm::romanticHall, { 29.000f, 3.500f, 0.920f, 26.000f, 98.000f, 116.000f, 5.600f, -3.300f, 84.000f, 90.000f, 13.000f, 34.000f, 13.000f, 145.000f, 9800.000f, 0.000f } },
        { "Bolero Warm", "Bolero", Algorithm::romanticHall, { 30.500f, 3.850f, 0.945f, 29.000f, 102.000f, 121.000f, 6.000f, -3.000f, 85.200f, 90.800f, 14.500f, 36.500f, 14.000f, 140.000f, 10250.000f, -0.250f } },
        { "Bolero Long", "Bolero", Algorithm::romanticHall, { 32.000f, 4.200f, 0.970f, 32.000f, 106.000f, 126.000f, 6.400f, -2.700f, 86.400f, 91.600f, 16.000f, 39.000f, 15.000f, 135.000f, 10700.000f, -0.500f } },
        { "Bolero Live", "Bolero", Algorithm::romanticHall, { 33.500f, 4.550f, 0.995f, 35.000f, 110.000f, 131.000f, 6.800f, -2.400f, 87.600f, 92.400f, 17.500f, 41.500f, 16.000f, 130.000f, 11150.000f, -0.750f } },
        { "Studio Tight", "Studio", Algorithm::studioRoom, { 24.000f, 1.500f, 0.800f, 12.000f, 68.000f, 94.000f, 0.600f, -0.300f, 72.000f, 76.000f, 6.000f, 12.000f, 8.000f, 160.000f, 14500.000f, 0.000f } },
        { "Studio Vocal", "Studio", Algorithm::studioRoom, { 25.500f, 1.850f, 0.825f, 15.000f, 72.000f, 99.000f, 1.000f, 0.000f, 73.200f, 76.800f, 7.500f, 14.500f, 9.000f, 155.000f, 14950.000f, -0.250f } },
        { "Studio Natural", "Studio", Algorithm::studioRoom, { 27.000f, 2.200f, 0.850f, 18.000f, 76.000f, 104.000f, 1.400f, 0.300f, 74.400f, 77.600f, 9.000f, 17.000f, 10.000f, 150.000f, 15400.000f, -0.500f } },
        { "Studio Wide", "Studio", Algorithm::studioRoom, { 28.500f, 2.550f, 0.875f, 21.000f, 80.000f, 109.000f, 1.800f, 0.600f, 75.600f, 78.400f, 10.500f, 19.500f, 11.000f, 145.000f, 15850.000f, -0.750f } },
        { "Vocal Plate", "Plate", Algorithm::vocalPlate, { 30.000f, 2.800f, 0.920f, 18.000f, 88.000f, 136.000f, 1.600f, 1.700f, 95.000f, 97.000f, 18.000f, 30.000f, 14.000f, 170.000f, 15500.000f, -0.500f } },
        { "Bright Plate", "Plate", Algorithm::vocalPlate, { 31.500f, 3.150f, 0.945f, 21.000f, 92.000f, 141.000f, 2.000f, 2.000f, 96.200f, 97.800f, 19.500f, 32.500f, 15.000f, 165.000f, 15950.000f, -0.750f } },
        { "Warm Plate", "Plate", Algorithm::vocalPlate, { 33.000f, 3.500f, 0.970f, 24.000f, 96.000f, 146.000f, 2.400f, 2.300f, 97.400f, 98.600f, 21.000f, 35.000f, 16.000f, 160.000f, 16400.000f, -1.000f } },
        { "Dense Plate", "Plate", Algorithm::vocalPlate, { 34.500f, 3.850f, 0.995f, 27.000f, 100.000f, 151.000f, 2.800f, 2.600f, 98.600f, 99.400f, 22.500f, 37.500f, 17.000f, 155.000f, 16850.000f, -1.250f } },
        { "Classic Hall", "Hall", Algorithm::romanticHall, { 38.000f, 5.200f, 1.050f, 45.000f, 132.000f, 148.000f, 1.600f, -1.300f, 92.000f, 96.000f, 25.000f, 55.000f, 20.000f, 105.000f, 12500.000f, -1.000f } },
        { "Grand Hall", "Hall", Algorithm::romanticHall, { 39.500f, 5.550f, 1.075f, 48.000f, 136.000f, 153.000f, 2.000f, -1.000f, 93.200f, 96.800f, 26.500f, 57.500f, 21.000f, 100.000f, 12950.000f, -1.250f } },
        { "Smooth Hall", "Hall", Algorithm::romanticHall, { 41.000f, 5.900f, 1.100f, 51.000f, 140.000f, 158.000f, 2.400f, -0.700f, 94.400f, 97.600f, 28.000f, 60.000f, 22.000f, 95.000f, 13400.000f, -1.500f } },
        { "Concert Hall", "Hall", Algorithm::romanticHall, { 42.500f, 6.250f, 1.125f, 54.000f, 144.000f, 163.000f, 2.800f, -0.400f, 95.600f, 98.400f, 29.500f, 62.500f, 23.000f, 90.000f, 13850.000f, -1.750f } },
        { "Small Chamber", "Chamber", Algorithm::chamber, { 30.000f, 2.600f, 0.900f, 20.000f, 86.000f, 112.000f, 3.600f, -1.300f, 88.000f, 90.000f, 12.000f, 28.000f, 12.000f, 145.000f, 11200.000f, 0.000f } },
        { "Vocal Chamber", "Chamber", Algorithm::chamber, { 31.500f, 2.950f, 0.925f, 23.000f, 90.000f, 117.000f, 4.000f, -1.000f, 89.200f, 90.800f, 13.500f, 30.500f, 13.000f, 140.000f, 11650.000f, -0.250f } },
        { "Warm Chamber", "Chamber", Algorithm::chamber, { 33.000f, 3.300f, 0.950f, 26.000f, 94.000f, 122.000f, 4.400f, -0.700f, 90.400f, 91.600f, 15.000f, 33.000f, 14.000f, 135.000f, 12100.000f, -0.500f } },
        { "Wide Chamber", "Chamber", Algorithm::chamber, { 34.500f, 3.650f, 0.975f, 29.000f, 98.000f, 127.000f, 4.800f, -0.400f, 91.600f, 92.400f, 16.500f, 35.500f, 15.000f, 130.000f, 12550.000f, -0.750f } },
        { "Sacred Space", "Cathedral", Algorithm::cathedral, { 45.000f, 8.400f, 1.180f, 72.000f, 165.000f, 176.000f, 2.600f, -3.300f, 96.000f, 98.000f, 32.000f, 74.000f, 24.000f, 85.000f, 9000.000f, -2.000f } },
        { "Dark Cathedral", "Cathedral", Algorithm::cathedral, { 46.500f, 8.750f, 1.205f, 75.000f, 169.000f, 181.000f, 3.000f, -3.000f, 97.200f, 98.800f, 33.500f, 76.500f, 25.000f, 80.000f, 9450.000f, -2.250f } },
        { "Bright Cathedral", "Cathedral", Algorithm::cathedral, { 48.000f, 9.100f, 1.230f, 78.000f, 173.000f, 186.000f, 3.400f, -2.700f, 98.400f, 99.600f, 35.000f, 79.000f, 26.000f, 75.000f, 9900.000f, -2.500f } },
        { "Huge Cathedral", "Cathedral", Algorithm::cathedral, { 49.500f, 9.450f, 1.255f, 81.000f, 177.000f, 191.000f, 3.800f, -2.400f, 99.600f, 100.400f, 36.500f, 81.500f, 27.000f, 70.000f, 10350.000f, -2.750f } },
        { "Ambient Cloud", "Ambient", Algorithm::ambient, { 52.000f, 10.500f, 1.280f, 85.000f, 178.000f, 188.000f, 0.600f, 0.700f, 98.000f, 99.000f, 44.000f, 86.000f, 12.000f, 70.000f, 13500.000f, -3.000f } },
        { "Infinite Dream", "Ambient", Algorithm::ambient, { 53.500f, 10.850f, 1.305f, 88.000f, 182.000f, 193.000f, 1.000f, 1.000f, 99.200f, 99.800f, 45.500f, 88.500f, 13.000f, 65.000f, 13950.000f, -3.250f } },
        { "Floating Air", "Ambient", Algorithm::ambient, { 55.000f, 11.200f, 1.330f, 91.000f, 186.000f, 198.000f, 1.400f, 1.300f, 100.400f, 100.600f, 47.000f, 91.000f, 14.000f, 60.000f, 14400.000f, -3.500f } },
        { "Deep Atmosphere", "Ambient", Algorithm::ambient, { 56.500f, 11.550f, 1.355f, 94.000f, 190.000f, 203.000f, 1.800f, 1.600f, 101.600f, 101.400f, 48.500f, 93.500f, 15.000f, 55.000f, 14850.000f, -3.750f } },
        { "Dream Vocal", "Dream", Algorithm::ambient, { 46.000f, 7.200f, 1.160f, 60.000f, 154.000f, 182.000f, 3.600f, -2.300f, 96.000f, 98.000f, 38.000f, 78.000f, 16.000f, 80.000f, 10500.000f, -2.000f } },
        { "Moonlight", "Dream", Algorithm::ambient, { 47.500f, 7.550f, 1.185f, 63.000f, 158.000f, 187.000f, 4.000f, -2.000f, 97.200f, 98.800f, 39.500f, 80.500f, 17.000f, 75.000f, 10950.000f, -2.250f } },
        { "Soft Horizon", "Dream", Algorithm::ambient, { 49.000f, 7.900f, 1.210f, 66.000f, 162.000f, 192.000f, 4.400f, -1.700f, 98.400f, 99.600f, 41.000f, 83.000f, 18.000f, 70.000f, 11400.000f, -2.500f } },
        { "Eternal Bloom", "Dream", Algorithm::ambient, { 50.500f, 8.250f, 1.235f, 69.000f, 166.000f, 197.000f, 4.800f, -1.400f, 99.600f, 100.400f, 42.500f, 85.500f, 19.000f, 65.000f, 11850.000f, -2.750f } },
        { "Live Small", "Live Vocal", Algorithm::studioRoom, { 22.000f, 1.800f, 0.860f, 14.000f, 76.000f, 104.000f, 1.600f, 0.700f, 78.000f, 82.000f, 8.000f, 16.000f, 28.000f, 170.000f, 15000.000f, 0.000f } },
        { "Live Medium", "Live Vocal", Algorithm::studioRoom, { 23.500f, 2.150f, 0.885f, 17.000f, 80.000f, 109.000f, 2.000f, 1.000f, 79.200f, 82.800f, 9.500f, 18.500f, 29.000f, 165.000f, 15450.000f, -0.250f } },
        { "Live Large", "Live Vocal", Algorithm::studioRoom, { 25.000f, 2.500f, 0.910f, 20.000f, 84.000f, 114.000f, 2.400f, 1.300f, 80.400f, 83.600f, 11.000f, 21.000f, 30.000f, 160.000f, 15900.000f, -0.500f } },
        { "Live Romantic", "Live Vocal", Algorithm::studioRoom, { 26.500f, 2.850f, 0.935f, 23.000f, 88.000f, 119.000f, 2.800f, 1.600f, 81.600f, 84.400f, 12.500f, 23.500f, 31.000f, 155.000f, 16350.000f, -0.750f } },
        { "Acoustic Room", "Acoustic", Algorithm::chamber, { 26.000f, 2.300f, 0.900f, 22.000f, 90.000f, 110.000f, 2.600f, -0.300f, 82.000f, 86.000f, 10.000f, 24.000f, 10.000f, 125.000f, 13200.000f, 0.000f } },
        { "Acoustic Hall", "Acoustic", Algorithm::chamber, { 27.500f, 2.650f, 0.925f, 25.000f, 94.000f, 115.000f, 3.000f, 0.000f, 83.200f, 86.800f, 11.500f, 26.500f, 11.000f, 120.000f, 13650.000f, -0.250f } },
        { "Guitar Space", "Acoustic", Algorithm::chamber, { 29.000f, 3.000f, 0.950f, 28.000f, 98.000f, 120.000f, 3.400f, 0.300f, 84.400f, 87.600f, 13.000f, 29.000f, 12.000f, 115.000f, 14100.000f, -0.500f } },
        { "Piano Romance", "Acoustic", Algorithm::chamber, { 30.500f, 3.350f, 0.975f, 31.000f, 102.000f, 125.000f, 3.800f, 0.600f, 85.600f, 88.400f, 14.500f, 31.500f, 13.000f, 110.000f, 14550.000f, -0.750f } }
    }};
    return presets;
}

int ReverbRomanticAudioProcessor::getNumFactoryPresets() const noexcept
{
    return static_cast<int> (getFactoryPresetTable().size());
}

juce::String ReverbRomanticAudioProcessor::getFactoryPresetName (int index) const
{
    const auto& presets = getFactoryPresetTable();
    return presets[static_cast<size_t> (juce::jlimit (0, getNumFactoryPresets() - 1, index))].name;
}

juce::String ReverbRomanticAudioProcessor::getFactoryPresetCategory (int index) const
{
    const auto& presets = getFactoryPresetTable();
    return presets[static_cast<size_t> (juce::jlimit (0, getNumFactoryPresets() - 1, index))].category;
}

juce::StringArray ReverbRomanticAudioProcessor::getFactoryPresetCategories() const
{
    juce::StringArray categories { "All", "Favorites" };
    for (const auto& preset : getFactoryPresetTable())
        categories.addIfNotAlreadyThere (preset.category);
    return categories;
}

void ReverbRomanticAudioProcessor::setParameterPlainValue (const juce::String& id, float plainValue)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plainValue));
}

void ReverbRomanticAudioProcessor::loadFactoryPreset (int index)
{
    pushUndoState();
    const int safeIndex = juce::jlimit (0, getNumFactoryPresets() - 1, index);
    const auto& preset = getFactoryPresetTable()[static_cast<size_t> (safeIndex)];

    if (auto* algorithm = apvts.getParameter (Parameters::IDs::algorithm))
        algorithm->setValueNotifyingHost (algorithm->convertTo0to1 (static_cast<float> (preset.algorithm)));

    for (size_t i = 0; i < preset.values.size(); ++i)
        setParameterPlainValue (presetParameterIds[i], preset.values[i]);

    currentFactoryPreset.store (safeIndex);
    apvts.state.setProperty ("factoryPresetIndex", safeIndex, nullptr);
}


juce::ValueTree ReverbRomanticAudioProcessor::createStateSnapshot()
{
    return apvts.copyState().createCopy();
}

void ReverbRomanticAudioProcessor::restoreStateSnapshot (const juce::ValueTree& snapshot)
{
    if (snapshot.isValid() && snapshot.hasType (apvts.state.getType()))
        apvts.replaceState (snapshot.createCopy());
}

void ReverbRomanticAudioProcessor::captureSnapshotA()
{
    snapshotA = createStateSnapshot();
    snapshotBActive.store (false);
}

void ReverbRomanticAudioProcessor::captureSnapshotB()
{
    snapshotB = createStateSnapshot();
    snapshotBActive.store (true);
}

void ReverbRomanticAudioProcessor::recallSnapshotA()
{
    pushUndoState();
    restoreStateSnapshot (snapshotA);
    snapshotBActive.store (false);
}

void ReverbRomanticAudioProcessor::recallSnapshotB()
{
    pushUndoState();
    restoreStateSnapshot (snapshotB);
    snapshotBActive.store (true);
}

void ReverbRomanticAudioProcessor::setFactoryPresetFavourite (int index,
                                                               bool shouldBeFavourite)
{
    const int safeIndex = juce::jlimit (0, getNumFactoryPresets() - 1, index);
    if (shouldBeFavourite)
        favouriteFactoryPresets.insert (safeIndex);
    else
        favouriteFactoryPresets.erase (safeIndex);
    storePersistentMetadata();
}

bool ReverbRomanticAudioProcessor::isFactoryPresetFavourite (int index) const
{
    return favouriteFactoryPresets.count (index) != 0;
}

juce::Array<int> ReverbRomanticAudioProcessor::getFavouriteFactoryPresets() const
{
    juce::Array<int> result;
    for (const auto index : favouriteFactoryPresets)
        result.add (index);
    return result;
}

juce::File ReverbRomanticAudioProcessor::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("ReverbRomantic")
        .getChildFile ("Presets");
}

juce::File ReverbRomanticAudioProcessor::getUserPresetFile (const juce::String& name) const
{
    auto safeName = juce::File::createLegalFileName (name.trim());
    if (safeName.isEmpty())
        safeName = "User Preset";
    return getUserPresetDirectory().getChildFile (safeName + ".rrpreset");
}

juce::StringArray ReverbRomanticAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    const auto directory = getUserPresetDirectory();
    if (! directory.isDirectory())
        return names;

    for (const auto& file : directory.findChildFiles (juce::File::findFiles, false, "*.rrpreset"))
        names.add (file.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool ReverbRomanticAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto directory = getUserPresetDirectory();
    if (! directory.exists() && directory.createDirectory().failed())
        return false;

    auto snapshot = createStateSnapshot();
    snapshot.setProperty ("userPresetName", name.trim(), nullptr);
    if (auto xml = snapshot.createXml())
        return xml->writeTo (getUserPresetFile (name));
    return false;
}

bool ReverbRomanticAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = getUserPresetFile (name);
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr)
        return false;

    const auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (apvts.state.getType()))
        return false;

    pushUndoState();
    restoreStateSnapshot (state);
    currentFactoryPreset.store (-1);
    apvts.state.setProperty ("factoryPresetIndex", -1, nullptr);
    return true;
}

bool ReverbRomanticAudioProcessor::deleteUserPreset (const juce::String& name)
{
    const auto file = getUserPresetFile (name);
    return ! file.existsAsFile() || file.deleteFile();
}

bool ReverbRomanticAudioProcessor::renameUserPreset (const juce::String& oldName,
                                                      const juce::String& newName)
{
    const auto source = getUserPresetFile (oldName);
    const auto destination = getUserPresetFile (newName);
    return source.existsAsFile() && source.moveFileTo (destination);
}

bool ReverbRomanticAudioProcessor::duplicateUserPreset (const juce::String& sourceName,
                                                         const juce::String& destinationName)
{
    const auto source = getUserPresetFile (sourceName);
    const auto destination = getUserPresetFile (destinationName);
    return source.existsAsFile() && source.copyFileTo (destination);
}

void ReverbRomanticAudioProcessor::pushUndoState()
{
    undoHistory.push_back (createStateSnapshot());
    if (undoHistory.size() > maxUndoStates)
        undoHistory.erase (undoHistory.begin());
    redoHistory.clear();
}

bool ReverbRomanticAudioProcessor::undoPresetChange()
{
    if (undoHistory.empty())
        return false;
    redoHistory.push_back (createStateSnapshot());
    const auto state = undoHistory.back();
    undoHistory.pop_back();
    restoreStateSnapshot (state);
    return true;
}

bool ReverbRomanticAudioProcessor::redoPresetChange()
{
    if (redoHistory.empty())
        return false;
    undoHistory.push_back (createStateSnapshot());
    const auto state = redoHistory.back();
    redoHistory.pop_back();
    restoreStateSnapshot (state);
    return true;
}

void ReverbRomanticAudioProcessor::loadPersistentMetadata()
{
    favouriteFactoryPresets.clear();
    const auto encoded = apvts.state.getProperty ("favouriteFactoryPresets", "").toString();
    juce::StringArray tokens;
    tokens.addTokens (encoded, ",", "");
    for (const auto& token : tokens)
    {
        const int index = token.getIntValue();
        if (juce::isPositiveAndBelow (index, getNumFactoryPresets()))
            favouriteFactoryPresets.insert (index);
    }
}

void ReverbRomanticAudioProcessor::storePersistentMetadata()
{
    juce::StringArray values;
    for (const auto index : favouriteFactoryPresets)
        values.add (juce::String (index));
    apvts.state.setProperty ("favouriteFactoryPresets", values.joinIntoString (","), nullptr);
}

void ReverbRomanticAudioProcessor::prepareToPlay (double sampleRate,
                                                   int samplesPerBlock)
{
    const juce::dsp::ProcessSpec spec { sampleRate,
                                        static_cast<juce::uint32> (samplesPerBlock),
                                        2 };
    engine.prepare (spec);
    freezeProcessor.prepare (spec);
    shimmer.prepare (spec);
    postDucking.prepare (spec);
    postWidth.prepare (spec);
    postLimiter.prepare (spec);
}

void ReverbRomanticAudioProcessor::releaseResources()
{
    engine.reset();
    freezeProcessor.reset();
    shimmer.reset();
    postDucking.reset();
    postWidth.reset();
    postLimiter.reset();
}

bool ReverbRomanticAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    if (mainInput != mainOutput)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);

        if (! sidechain.isDisabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

double ReverbRomanticAudioProcessor::getTailLengthSeconds() const
{
    const float decay = apvts.getRawParameterValue (Parameters::IDs::decay)->load();
    const float time = apvts.getRawParameterValue (Parameters::IDs::time)->load();
    const auto algorithm = sanitiseAlgorithm (
        apvts.getRawParameterValue (Parameters::IDs::algorithm)->load());

    float algorithmScale = 1.0f;
    switch (algorithm)
    {
        case Algorithm::romanticHall: algorithmScale = 1.12f; break;
        case Algorithm::vocalPlate:   algorithmScale = 1.04f; break;
        case Algorithm::studioRoom:   algorithmScale = 0.82f; break;
        case Algorithm::chamber:      algorithmScale = 0.96f; break;
        case Algorithm::cathedral:    algorithmScale = 1.30f; break;
        case Algorithm::ambient:      algorithmScale = 1.55f; break;
    }

    return static_cast<double> (decay * time * algorithmScale);
}

ReverbRomanticAudioProcessor::Algorithm
ReverbRomanticAudioProcessor::sanitiseAlgorithm (float rawValue) noexcept
{
    const int index = juce::jlimit (0, 5, static_cast<int> (rawValue));
    return static_cast<Algorithm> (index);
}

void ReverbRomanticAudioProcessor::applyAlgorithmProfile (
    Algorithm algorithm,
    HybridFDN16::Parameters& p) noexcept
{
    // Profiles shape the DSP internally without overwriting user automation.
    // Every visible control remains effective around the selected algorithm.
    switch (algorithm)
    {
        case Algorithm::romanticHall:
            p.roomModel = HybridFDN16::RoomModel::hall;
            p.decaySeconds *= 1.12f;
            p.preDelayMs *= 1.05f;
            p.sizePercent *= 1.08f;
            p.widthPercent *= 1.06f;
            p.diffusionPercent += 4.0f;
            p.densityPercent += 3.0f;
            p.modulationPercent += 4.0f;
            p.bloomPercent += 8.0f;
            p.warmthDb += 0.8f;
            break;

        case Algorithm::vocalPlate:
            p.roomModel = HybridFDN16::RoomModel::plate;
            p.decaySeconds *= 0.94f;
            p.preDelayMs *= 0.72f;
            p.sizePercent *= 0.88f;
            p.widthPercent *= 1.08f;
            p.diffusionPercent += 8.0f;
            p.densityPercent += 5.0f;
            p.modulationPercent += 3.0f;
            p.bloomPercent += 2.0f;
            p.lowCutHz += 35.0f;
            p.brightnessDb += 1.2f;
            break;

        case Algorithm::studioRoom:
            p.roomModel = HybridFDN16::RoomModel::studio;
            p.decaySeconds *= 0.72f;
            p.preDelayMs *= 0.45f;
            p.sizePercent *= 0.72f;
            p.widthPercent *= 0.88f;
            p.diffusionPercent -= 7.0f;
            p.densityPercent -= 9.0f;
            p.modulationPercent *= 0.55f;
            p.bloomPercent *= 0.45f;
            p.highCutHz *= 0.92f;
            break;

        case Algorithm::chamber:
            p.roomModel = HybridFDN16::RoomModel::chamber;
            p.decaySeconds *= 0.92f;
            p.preDelayMs *= 0.82f;
            p.sizePercent *= 0.92f;
            p.widthPercent *= 0.96f;
            p.diffusionPercent += 1.0f;
            p.densityPercent += 1.0f;
            p.modulationPercent *= 0.85f;
            p.bloomPercent *= 0.82f;
            p.warmthDb += 0.4f;
            break;

        case Algorithm::cathedral:
            p.roomModel = HybridFDN16::RoomModel::cathedral;
            p.decaySeconds *= 1.30f;
            p.preDelayMs *= 1.28f;
            p.sizePercent *= 1.20f;
            p.widthPercent *= 1.10f;
            p.diffusionPercent += 5.0f;
            p.densityPercent += 4.0f;
            p.modulationPercent += 3.0f;
            p.bloomPercent += 12.0f;
            p.highCutHz *= 0.86f;
            p.warmthDb += 0.5f;
            break;

        case Algorithm::ambient:
            p.roomModel = HybridFDN16::RoomModel::cathedral;
            p.decaySeconds *= 1.55f;
            p.preDelayMs *= 1.18f;
            p.sizePercent *= 1.26f;
            p.widthPercent *= 1.18f;
            p.diffusionPercent += 8.0f;
            p.densityPercent += 6.0f;
            p.modulationPercent += 10.0f;
            p.bloomPercent += 22.0f;
            p.duckingPercent *= 0.65f;
            p.highCutHz *= 0.92f;
            p.brightnessDb += 0.6f;
            break;
    }

    // Keep profile offsets inside the ranges accepted by HybridFDN16.
    p.decaySeconds      = juce::jlimit (0.2f, 60.0f, p.decaySeconds);
    p.preDelayMs        = juce::jlimit (0.0f, 250.0f, p.preDelayMs);
    p.sizePercent       = juce::jlimit (25.0f, 200.0f, p.sizePercent);
    p.widthPercent      = juce::jlimit (0.0f, 200.0f, p.widthPercent);
    p.diffusionPercent  = juce::jlimit (0.0f, 100.0f, p.diffusionPercent);
    p.densityPercent    = juce::jlimit (0.0f, 100.0f, p.densityPercent);
    p.modulationPercent = juce::jlimit (0.0f, 100.0f, p.modulationPercent);
    p.bloomPercent      = juce::jlimit (0.0f, 100.0f, p.bloomPercent);
    p.duckingPercent    = juce::jlimit (0.0f, 100.0f, p.duckingPercent);
    p.lowCutHz          = juce::jlimit (20.0f, 1000.0f, p.lowCutHz);
    p.highCutHz         = juce::jlimit (1000.0f, 20000.0f, p.highCutHz);
    p.warmthDb          = juce::jlimit (-12.0f, 12.0f, p.warmthDb);
    p.brightnessDb      = juce::jlimit (-12.0f, 12.0f, p.brightnessDb);
}

void ReverbRomanticAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const bool hasSidechainBus = getBusCount (true) > 1
                                 && getBus (true, 1) != nullptr
                                 && getBus (true, 1)->isEnabled();

    if (hasSidechainBus)
    {
        const auto sidechainBuffer = getBusBuffer (buffer, true, 1);
        const int sidechainChannels = sidechainBuffer.getNumChannels();

        if (sidechainChannels > 0)
        {
            float peak = sidechainBuffer.getMagnitude (0, 0, sidechainBuffer.getNumSamples());

            if (sidechainChannels > 1)
                peak = juce::jmax (peak,
                                   sidechainBuffer.getMagnitude (1, 0,
                                                                 sidechainBuffer.getNumSamples()));

            sidechainPeak.store (peak);
            sidechainConnected.store (true);
        }
        else
        {
            sidechainPeak.store (0.0f);
            sidechainConnected.store (false);
        }
    }
    else
    {
        sidechainPeak.store (0.0f);
        sidechainConnected.store (false);
    }

    const auto rightChannel = juce::jmin (1, buffer.getNumChannels() - 1);
    inputPeakL.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    inputPeakR.store (buffer.getMagnitude (rightChannel, 0, buffer.getNumSamples()));

    if (apvts.getRawParameterValue (Parameters::IDs::bypass)->load() > 0.5f)
    {
        outputPeakL.store (inputPeakL.load());
        outputPeakR.store (inputPeakR.load());
        return;
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    const float mix = apvts.getRawParameterValue (Parameters::IDs::mix)->load() * 0.01f;
    const float outputGain = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (Parameters::IDs::output)->load());

    HybridFDN16::Parameters dspParameters;
    dspParameters.decaySeconds =
        apvts.getRawParameterValue (Parameters::IDs::decay)->load()
        * apvts.getRawParameterValue (Parameters::IDs::time)->load();
    dspParameters.preDelayMs = apvts.getRawParameterValue (Parameters::IDs::preDelay)->load();
    dspParameters.sizePercent = apvts.getRawParameterValue (Parameters::IDs::size)->load();
    dspParameters.widthPercent = apvts.getRawParameterValue (Parameters::IDs::width)->load();
    dspParameters.diffusionPercent = apvts.getRawParameterValue (Parameters::IDs::diffusion)->load();
    dspParameters.densityPercent = apvts.getRawParameterValue (Parameters::IDs::density)->load();
    dspParameters.modulationPercent = apvts.getRawParameterValue (Parameters::IDs::modulation)->load();
    dspParameters.bloomPercent = apvts.getRawParameterValue (Parameters::IDs::bloom)->load();
    // Epic 5C.3: ducking is applied after Freeze and Shimmer.
    dspParameters.duckingPercent = 0.0f;

    dspParameters.sidechainEnabled =
        apvts.getRawParameterValue (Parameters::IDs::sidechainEnable)->load() > 0.5f;
    dspParameters.sidechainThresholdDb =
        apvts.getRawParameterValue (Parameters::IDs::sidechainThreshold)->load();
    dspParameters.sidechainAmountPercent =
        apvts.getRawParameterValue (Parameters::IDs::sidechainAmount)->load();
    dspParameters.sidechainAttackMs =
        apvts.getRawParameterValue (Parameters::IDs::sidechainAttack)->load();
    dspParameters.sidechainReleaseMs =
        apvts.getRawParameterValue (Parameters::IDs::sidechainRelease)->load();
    dspParameters.sidechainHighPassHz =
        apvts.getRawParameterValue (Parameters::IDs::sidechainHPF)->load();

    dspParameters.lowCutHz = apvts.getRawParameterValue (Parameters::IDs::lowCut)->load();
    dspParameters.highCutHz = apvts.getRawParameterValue (Parameters::IDs::highCut)->load();
    dspParameters.warmthDb = apvts.getRawParameterValue (Parameters::IDs::warmth)->load();
    dspParameters.brightnessDb = apvts.getRawParameterValue (Parameters::IDs::brightness)->load();
    dspParameters.quality = static_cast<int> (
        apvts.getRawParameterValue (Parameters::IDs::quality)->load());
    dspParameters.freeze =
        apvts.getRawParameterValue (Parameters::IDs::freeze)->load() > 0.5f;
    dspParameters.processOutputStage = false;

    const auto algorithm = sanitiseAlgorithm (
        apvts.getRawParameterValue (Parameters::IDs::algorithm)->load());
    applyAlgorithmProfile (algorithm, dspParameters);
    engine.setParameters (dspParameters);

    ShimmerProcessor::Parameters shimmerParameters;
    shimmerParameters.enabled =
        apvts.getRawParameterValue (Parameters::IDs::shimmerEnable)->load() > 0.5f;
    shimmerParameters.mixPercent =
        apvts.getRawParameterValue (Parameters::IDs::shimmerMix)->load();
    shimmerParameters.pitchSemitones =
        apvts.getRawParameterValue (Parameters::IDs::shimmerPitch)->load() < 0.5f
            ? 7.0f
            : 12.0f;
    shimmerParameters.feedbackPercent =
        apvts.getRawParameterValue (Parameters::IDs::shimmerFeedback)->load();
    shimmerParameters.toneHz =
        apvts.getRawParameterValue (Parameters::IDs::shimmerTone)->load();
    shimmer.setParameters (shimmerParameters);

    FreezeProcessor::Parameters freezeParameters;
    freezeParameters.enabled =
        apvts.getRawParameterValue (Parameters::IDs::freeze)->load() > 0.5f;
    freezeParameters.mixPercent =
        apvts.getRawParameterValue (Parameters::IDs::freezeMix)->load();
    freezeParameters.fadeTimeMs =
        apvts.getRawParameterValue (Parameters::IDs::freezeFade)->load();
    freezeParameters.damping =
        apvts.getRawParameterValue (Parameters::IDs::freezeDamp)->load() * 0.01f;
    freezeProcessor.setParameters (freezeParameters);

    postDucking.setEnabled (
        apvts.getRawParameterValue (Parameters::IDs::duckingEnable)->load() > 0.5f);
    postDucking.setThresholdDb (
        apvts.getRawParameterValue (Parameters::IDs::duckThreshold)->load());
    postDucking.setDepth (
        apvts.getRawParameterValue (Parameters::IDs::duckDepth)->load());
    postDucking.setAttackMs (
        apvts.getRawParameterValue (Parameters::IDs::duckAttack)->load());
    postDucking.setReleaseMs (
        apvts.getRawParameterValue (Parameters::IDs::duckRelease)->load());
    postDucking.setKneeDb (
        apvts.getRawParameterValue (Parameters::IDs::duckKnee)->load());

    postWidth.setWidth (dspParameters.widthPercent);

    const bool isMono = buffer.getNumChannels() == 1;

    
    const bool useExternalSidechain =
        dspParameters.sidechainEnabled
        && getBusCount (true) > 1
        && getBus (true, 1) != nullptr
        && getBus (true, 1)->isEnabled();

    juce::AudioBuffer<float> sidechainBuffer;
    const float* sidechainLeft = nullptr;
    const float* sidechainRight = nullptr;

    if (useExternalSidechain)
    {
        sidechainBuffer = getBusBuffer (buffer, true, 1);

        if (sidechainBuffer.getNumChannels() > 0)
        {
            sidechainLeft = sidechainBuffer.getReadPointer (0);
            sidechainRight = sidechainBuffer.getNumChannels() > 1
                               ? sidechainBuffer.getReadPointer (1)
                               : sidechainLeft;
        }
    }

for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float dryL = left[sample];
        const float dryR = isMono ? dryL : right[sample];
        float wetL = 0.0f;
        float wetR = 0.0f;
        if (sidechainLeft != nullptr)
        {
            engine.processStereo (dryL, dryR,
                                  sidechainLeft[sample],
                                  sidechainRight[sample],
                                  wetL, wetR);
        }
        else
        {
            engine.processStereo (dryL, dryR, wetL, wetR);
        }

        float frozenL = wetL;
        float frozenR = wetR;
        freezeProcessor.processStereo (wetL, wetR, frozenL, frozenR);
        wetL = frozenL;
        wetR = frozenR;

        float shimmeredL = wetL;
        float shimmeredR = wetR;
        shimmer.processStereo (wetL, wetR, shimmeredL, shimmeredR);
        wetL = shimmeredL;
        wetR = shimmeredR;

        // Fixed Epic 5 pipeline:
        // FDN -> Freeze -> Shimmer -> Ducking -> Width -> Limiter.
        postDucking.process (dryL, dryR, wetL, wetR);
        postWidth.process (wetL, wetR);
        postLimiter.process (wetL, wetR);

        if (isMono)
        {
            const float wetMono = 0.5f * (wetL + wetR);
            left[sample] = (dryL * (1.0f - mix) + wetMono * mix) * outputGain;
        }
        else
        {
            left[sample] = (dryL * (1.0f - mix) + wetL * mix) * outputGain;
            right[sample] = (dryR * (1.0f - mix) + wetR * mix) * outputGain;
        }
    }

    outputPeakL.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    outputPeakR.store (buffer.getMagnitude (rightChannel, 0, buffer.getNumSamples()));
}

juce::AudioProcessorEditor* ReverbRomanticAudioProcessor::createEditor()
{
    return new ReverbRomanticAudioProcessorEditor (*this);
}

void ReverbRomanticAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    storePersistentMetadata();
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void ReverbRomanticAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentFactoryPreset.store (juce::jlimit (-1, getNumFactoryPresets() - 1,
                static_cast<int> (apvts.state.getProperty ("factoryPresetIndex", 0))));
            loadPersistentMetadata();
            snapshotA = createStateSnapshot();
            snapshotB = snapshotA.createCopy();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbRomanticAudioProcessor();
}

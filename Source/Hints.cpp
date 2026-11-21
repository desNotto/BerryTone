// This file is UTF-8. MSVC compiles it with /utf-8 (see CMakeLists.txt).
#include "Hints.h"

namespace hints
{
namespace
{
    // Facts from Dsp.h and the parameter layout (ranges, filters, times).
    const char* const moduleRu[] = {
        "Фильтр высоких частот и компрессор полосы ниже 160 Гц.",
        "Гейт. Детектор с фильтром 110 Гц. Открытие 0.4 мс, удержание 12 мс.",
        "Транзиент-шейпер. Детектор работает в полосе 90 Гц – 3 кГц.",
        "Генератор октавы вниз: делитель частоты на 2. Для одиночных нот.",
        "Резонансный фильтр низких частот, 1.5–10 кГц, с регулировкой пика.",
        "Низкий и высокий шельф вокруг Pivot. Они меняют уровень в разные стороны.",
        "Клиппер, складывание волны, даунсемплинг и снижение разрядности.",
        "Усиление на входе и на выходе. Индикаторы уровня IN и OUT." };

    const char* const moduleEn[] = {
        "High-pass filter and a compressor for the band below 160 Hz.",
        "Noise gate. Detector with a 110 Hz filter. Opens in 0.4 ms, holds 12 ms.",
        "Transient shaper. The detector uses the 90 Hz - 3 kHz band.",
        "Sub-octave generator: frequency divider by 2. For single notes.",
        "Resonant low-pass filter, 1.5-10 kHz, with an adjustable peak.",
        "Low shelf and high shelf around Pivot. They change the level in opposite directions.",
        "Clipper, wave folder, sample-rate and bit-depth reduction.",
        "Input and output gain. IN and OUT level meters." };

    const char* const knobRu[8][3] = {
        { "Фильтр высоких частот, 12 дБ/окт. Срезает частоты ниже значения.",
          "Компрессор полосы ниже 160 Гц. Больше значение — сильнее сжатие низа.", nullptr },
        { "Уровень открытия гейта. Гейт закрывается на 4 дБ ниже этого уровня.",
          "Время закрытия гейта, 10–500 мс.",
          "Ослабление сигнала при закрытом гейте, от −80 до −6 дБ." },
        { "Усиление атаки ноты, до ±12 дБ. Минус ослабляет атаку.",
          "Усиление хвоста ноты. Плюс удлиняет ноту, минус укорачивает.",
          "Скорость детектора атаки, 0.5–2x. Больше — короче время реакции." },
        { "Уровень октавы вниз, 0–100 %.",
          "Фильтр низких частот октавы, 60–600 Гц. Меньше — меньше верха в октаве.",
          "Фильтр детектора высоты, 120–800 Гц. Для низкого строя ставьте меньше." },
        { "Частота среза фильтра низких частот, 1.5–10 кГц.",
          "Резонанс на частоте Freq. Больше значение — выше пик.",
          "Доля обработанного сигнала, 0–100 %. 0 % — без изменений." },
        { "Наклон АЧХ, ±8 дБ. Плюс поднимает верх и опускает низ. Минус — наоборот.",
          "Центр наклона, 200 Гц – 3 кГц. На этой частоте уровень не меняется.", nullptr },
        { "Усиление перед клиппером. Выше 40 % модуль также складывает волну.",
          "Снижение частоты дискретизации и разрядности. 0 % — выключено.",
          "Доля обработанного сигнала, 0–100 %." },
        { "Усиление на входе, ±24 дБ. Пики на входе: от −12 до −6 dBFS.",
          "Усиление на выходе, от −24 до +12 дБ.", nullptr } };

    const char* const knobEn[8][3] = {
        { "High-pass filter, 12 dB/octave. Removes frequencies below the value.",
          "Compressor for the band below 160 Hz. Higher value = more compression.", nullptr },
        { "Gate open level. The gate closes 4 dB below this level.",
          "Gate close time, 10-500 ms.",
          "Signal decrease when the gate is closed, -80 to -6 dB." },
        { "Attack gain, up to ±12 dB. Minus decreases the attack.",
          "Note tail gain. Plus makes the note longer, minus makes it shorter.",
          "Attack detector speed, 0.5-2x. Higher = shorter reaction time." },
        { "Sub-octave level, 0-100 %.",
          "Low-pass filter on the sub-octave, 60-600 Hz. Lower = fewer highs.",
          "Pitch detector filter, 120-800 Hz. Use a lower value for low tunings." },
        { "Low-pass filter cutoff, 1.5-10 kHz.",
          "Resonance at Freq. Higher value = higher peak.",
          "Processed signal amount, 0-100 %. 0 % = no change." },
        { "Tilt, ±8 dB. Plus increases the highs and decreases the lows. Minus: opposite.",
          "Tilt centre, 200 Hz - 3 kHz. The level at this frequency does not change.", nullptr },
        { "Gain before the clipper. Above 40 %, the module also folds the wave.",
          "Sample-rate and bit-depth decrease. 0 % = off.",
          "Processed signal amount, 0-100 %." },
        { "Input gain, ±24 dB. Set input peaks to -12 ... -6 dBFS.",
          "Output gain, -24 to +12 dB.", nullptr } };

    juce::String utf8 (const char* s) { return s != nullptr ? juce::String::fromUTF8 (s) : juce::String(); }

    juce::PropertiesFile& settings()
    {
        static juce::PropertiesFile file ([]
        {
            juce::PropertiesFile::Options o;
            o.applicationName = "settings";
            o.filenameSuffix = ".xml";
            o.folderName = "desNotto" + juce::String (juce::File::getSeparatorString()) + "BerryTone";
            o.osxLibrarySubFolder = "Application Support";
            o.storageFormat = juce::PropertiesFile::storeAsXML;
            o.millisecondsBeforeSaving = 0;
            return o;
        }());
        return file;
    }

    std::atomic<int> cached { -1 };
}

Lang getLang()
{
    if (cached.load() < 0)
        cached = settings().getValue ("hintLanguage", "ru") == "en" ? 1 : 0;
    return cached.load() == 1 ? Lang::en : Lang::ru;
}

void setLang (Lang l)
{
    cached = l == Lang::en ? 1 : 0;
    settings().setValue ("hintLanguage", l == Lang::en ? "en" : "ru");
    settings().saveIfNeeded();
}

juce::String moduleText (Lang l, int id)
{
    if (! juce::isPositiveAndBelow (id, 8)) return {};
    return utf8 (l == Lang::en ? moduleEn[id] : moduleRu[id]);
}

juce::String knobText (Lang l, int id, int k)
{
    if (! juce::isPositiveAndBelow (id, 8) || ! juce::isPositiveAndBelow (k, 3)) return {};
    return utf8 (l == Lang::en ? knobEn[id][k] : knobRu[id][k]);
}

juce::String idleText (Lang l)
{
    return utf8 (l == Lang::en ? "Point at a module or a knob to see what it does. Drag a card header to reorder."
                               : "Наведите на модуль или на ручку. Тяните заголовок карточки, чтобы сменить порядок.");
}
} // namespace hints

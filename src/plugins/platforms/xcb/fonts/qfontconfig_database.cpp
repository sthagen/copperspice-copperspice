/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
*
* Copyright (c) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#include <qfontconfig_database_p.h>

#include <qapplication.h>
#include <qelapsedtimer.h>
#include <qlist.h>
#include <qplatform_integration.h>
#include <qplatform_nativeinterface.h>
#include <qplatform_screen.h>
#include <qplatform_services.h>

#include <qapplication_p.h>
#include <qfontengine_ft_p.h>
#include <qfontengine_multifontconfig_p.h>
#include <qhighdpiscaling_p.h>

// unix library
#include <fontconfig/fontconfig.h>

#if FC_VERSION >= 20402
#include <fontconfig/fcfreetype.h>
#endif

static const int maxWeight = 99;

static inline int mapToQtWeightForRange(int fcweight, int fcLower, int fcUpper, int qtLower, int qtUpper)
{
   return qtLower + ((fcweight - fcLower) * (qtUpper - qtLower)) / (fcUpper - fcLower);
}

static inline int weightFromFcWeight(int fcweight)
{
   // Font Config uses weights from 0 to 215 (the highest enum value) while QFont ranges from
   // 0 to 99. The spacing between the values for the enums are uneven so a linear mapping from
   // Font Config values to CSt would give surprising results.  So, we do a piecewise linear
   // mapping.  This ensures that where there is a corresponding enum on both sides (for example
   // FC_WEIGHT_DEMIBOLD and QFont::DemiBold) we map one to the other but other values map to intermediate weights.

   if (fcweight <= FC_WEIGHT_THIN) {
      return QFont::Thin;
   }

   if (fcweight <= FC_WEIGHT_ULTRALIGHT) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_THIN, FC_WEIGHT_ULTRALIGHT, QFont::Thin, QFont::ExtraLight);
   }

   if (fcweight <= FC_WEIGHT_LIGHT) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_ULTRALIGHT, FC_WEIGHT_LIGHT, QFont::ExtraLight, QFont::Light);
   }

   if (fcweight <= FC_WEIGHT_NORMAL) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_LIGHT, FC_WEIGHT_NORMAL, QFont::Light, QFont::Normal);
   }

   if (fcweight <= FC_WEIGHT_MEDIUM) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_NORMAL, FC_WEIGHT_MEDIUM, QFont::Normal, QFont::Medium);
   }

   if (fcweight <= FC_WEIGHT_DEMIBOLD) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_MEDIUM, FC_WEIGHT_DEMIBOLD, QFont::Medium, QFont::DemiBold);
   }

   if (fcweight <= FC_WEIGHT_BOLD) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_DEMIBOLD, FC_WEIGHT_BOLD, QFont::DemiBold, QFont::Bold);
   }

   if (fcweight <= FC_WEIGHT_ULTRABOLD) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_BOLD, FC_WEIGHT_ULTRABOLD, QFont::Bold, QFont::ExtraBold);
   }

   if (fcweight <= FC_WEIGHT_BLACK) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_ULTRABOLD, FC_WEIGHT_BLACK, QFont::ExtraBold, QFont::Black);
   }

   if (fcweight <= FC_WEIGHT_ULTRABLACK) {
      return mapToQtWeightForRange(fcweight, FC_WEIGHT_BLACK, FC_WEIGHT_ULTRABLACK, QFont::Black, maxWeight);
   }

   return maxWeight;
}

static inline int stretchFromFcWidth(int fcwidth)
{
   // Font Config enums for width match pretty closely with those used by CS so just use
   // Font Config values directly while enforcing the same limits imposed by QFont.
   const int maxStretch = 4000;
   int qtstretch;

   if (fcwidth < 1) {
      qtstretch = 1;

   } else if (fcwidth > maxStretch) {
      qtstretch = maxStretch;

   } else {
      qtstretch = fcwidth;
   }

   return qtstretch;
}

static const char specialLanguages[][6] = {
   "",        // Unknown
   "",        // Inherited
   "",        // Common

   "en",      // Latin
   "el",      // Greek
   "ru",      // Cyrillic
   "hy",      // Armenian
   "he",      // Hebrew
   "ar",      // Arabic
   "syr",     // Syriac
   "dv",      // Thaana
   "hi",      // Devanagari
   "bn",      // Bengali
   "pa",      // Gurmukhi
   "gu",      // Gujarati
   "or",      // Oriya
   "ta",      // Tamil
   "te",      // Telugu
   "kn",      // Kannada
   "ml",      // Malayalam
   "si",      // Sinhala
   "th",      // Thai
   "lo",      // Lao
   "bo",      // Tibetan
   "my",      // Myanmar
   "ka",      // Georgian
   "ko",      // Hangul
   "am",      // Ethiopic
   "chr",     // Cherokee
   "cr",      // CanadianAboriginal

   "sga",     // Ogham
   "non",     // Runic
   "km",      // Khmer
   "mn",      // Mongolian
   "ja",      // Hiragana
   "ja",      // Katakana
   "zh-TW",   // Bopomofo
   "",        // Han
   "ii",      // Yi
   "ett",     // OldItalic
   "got",     // Gothic
   "en",      // Deseret
   "fil",     // Tagalog
   "hnn",     // Hanunoo
   "bku",     // Buhid
   "tbw",     // Tagbanwa
   "cop",     // Coptic

   "lif",     // Limbu
   "tdd",     // TaiLe
   "grc",     // LinearB
   "uga",     // Ugaritic
   "en",      // Shavian
   "so",      // Osmanya
   "grc",     // Cypriot
   "",        // Braille

   "bug",     // Buginese
   "khb",     // NewTaiLue
   "cu",      // Glagolitic
   "shi",     // Tifinagh
   "syl",     // SylotiNagri
   "peo",     // OldPersian
   "pra",     // Kharoshthi

   "ban",     // Balinese
   "akk",     // Cuneiform
   "phn",     // Phoenician
   "lzh",     // PhagsPa
   "man",     // Nko

   "su",      // Sundanese
   "lep",     // Lepcha
   "sat",     // OlChiki
   "vai",     // Vai
   "saz",     // Saurashtra
   "eky",     // KayahLi
   "rej",     // Rejang
   "xlc",     // Lycian
   "xcr",     // Carian
   "xld",     // Lydian
   "cjm",     // Cham

   "nod",     // TaiTham
   "blt",     // TaiViet
   "ae",      // Avestan
   "egy",     // EgyptianHieroglyphs
   "smp",     // Samaritan
   "lis",     // Lisu
   "bax",     // Bamum
   "jv",      // Javanese
   "mni",     // MeeteiMayek
   "arc",     // ImperialAramaic
   "xsa",     // OldSouthArabian
   "xpr",     // InscriptionalParthian
   "pal",     // InscriptionalPahlavi
   "otk",     // OldTurkic
   "bh",      // Kaithi

   "bbc",     // Batak
   "pra",     // Brahmi
   "myz",     // Mandaic

   "ccp",     // Chakma
   "xmr",     // MeroiticCursive
   "xmr",     // MeroiticHieroglyphs
   "hmd",     // Miao
   "sa",      // Sharada
   "srb",     // SoraSompeng
   "doi",     // Takri

   "lez",     // CaucasianAlbanian
   "bsq",     // BassaVah
   "fr",      // Duployan
   "sq",      // Elbasan
   "sa",      // Grantha
   "hnj",     // PahawhHmong
   "sd",      // Khojki
   "lab",     // LinearA
   "hi",      // Mahajani
   "xmn",     // Manichaean
   "men",     // MendeKikakui
   "mr",      // Modi
   "mru",     // Mro
   "xna",     // OldNorthArabian
   "arc",     // Nabataean
   "arc",     // Palmyrene
   "ctd",     // PauCinHau
   "kv",      // OldPermic
   "pal",     // PsalterPahlavi
   "sa",      // Siddham
   "sd",      // Khudawadi
   "mai",     // Tirhuta
   "hoc",     // WarangCiti

   "",        // unicode 8.0
   "",
   "",
   "",
   "",
   "",

   "",        // unicode 9.0
   "",
   "",
   "",
   "",
   "",

   "",        // unicode 10.0
   "",
   "",
   "",

   "",        // unicode 11.0
   "",
   "",
   "",
   "",
   "",
   "",

   "",        // unicode 12.0
   "",
   "",
   "",

   "",        // unicode 13.0
   "",
   "",
   "",

   "",        // unicode 14.0
   "",
   "",
   "",
   "",

   "",        // unicode 15.1
   "",

};
static_assert(sizeof(specialLanguages) / sizeof(* specialLanguages) == QChar::ScriptCount, "");

// could become a list of all languages used for each writing
// system, instead of using the single most common language.
static const char languageForWritingSystem[][6] = {
   "",        // Any
   "en",      // Latin
   "el",      // Greek
   "ru",      // Cyrillic
   "hy",      // Armenian
   "he",      // Hebrew
   "ar",      // Arabic
   "syr",     // Syriac
   "div",     // Thaana
   "hi",      // Devanagari
   "bn",      // Bengali
   "pa",      // Gurmukhi
   "gu",      // Gujarati
   "or",      // Oriya
   "ta",      // Tamil
   "te",      // Telugu
   "kn",      // Kannada
   "ml",      // Malayalam
   "si",      // Sinhala
   "th",      // Thai
   "lo",      // Lao
   "bo",      // Tibetan
   "my",      // Myanmar
   "ka",      // Georgian
   "km",      // Khmer
   "zh-cn",   // SimplifiedChinese
   "zh-tw",   // TraditionalChinese
   "ja",      // Japanese
   "ko",      // Korean
   "vi",      // Vietnamese
   "",        // Symbol
   "sga",     // Ogham
   "non",     // Runic
   "man"      // N'Ko
};
static_assert(sizeof(languageForWritingSystem) / sizeof(* languageForWritingSystem) == QFontDatabase::WritingSystemsCount, "");

#if FC_VERSION >= 20297

// Using newer FontConfig, sort out fonts which report support for certain scripts,
// but no open type tables for handling them correctly.
// Check the reported script presence in the FC_CAPABILITY's "otlayout:" section.

static const char capabilityForWritingSystem[][5] = {
   "",        // Any
   "",        // Latin
   "",        // Greek
   "",        // Cyrillic
   "",        // Armenian
   "",        // Hebrew
   "",        // Arabic
   "syrc",    // Syriac
   "thaa",    // Thaana
   "deva",    // Devanagari
   "beng",    // Bengali
   "guru",    // Gurmukhi
   "gujr",    // Gujarati
   "orya",    // Oriya
   "taml",    // Tamil
   "telu",    // Telugu
   "knda",    // Kannada
   "mlym",    // Malayalam
   "sinh",    // Sinhala
   "",        // Thai
   "",        // Lao
   "tibt",    // Tibetan
   "mymr",    // Myanmar
   "",        // Georgian
   "khmr",    // Khmer
   "",        // SimplifiedChinese
   "",        // TraditionalChinese
   "",        // Japanese
   "",        // Korean
   "",        // Vietnamese
   "",        // Symbol
   "",        // Ogham
   "",        // Runic
   "nko "     // N'Ko
};
static_assert(sizeof(capabilityForWritingSystem) / sizeof(*capabilityForWritingSystem) == QFontDatabase::WritingSystemsCount, "");
#endif

static const char *getFcFamilyForStyleHint(const QFont::StyleHint style)
{
   const char *stylehint = nullptr;

   switch (style) {
      case QFont::SansSerif:
         stylehint = "sans-serif";
         break;

      case QFont::Serif:
         stylehint = "serif";
         break;

      case QFont::TypeWriter:
      case QFont::Monospace:
         stylehint = "monospace";
         break;

      case QFont::Cursive:
         stylehint = "cursive";
         break;

      case QFont::Fantasy:
         stylehint = "fantasy";
         break;

      default:
         break;
   }

   return stylehint;
}

static inline bool requiresOpenType(int writingSystem)
{
   return ((writingSystem >= QFontDatabase::Syriac && writingSystem <= QFontDatabase::Sinhala)
         || writingSystem == QFontDatabase::Khmer || writingSystem == QFontDatabase::Nko);
}

static void populateFromPattern(FcPattern *pattern)
{
   QString familyName;

   FcChar8 *value = nullptr;
   int weight_value;
   int slant_value;
   int spacing_value;
   int width_value;
   FcChar8 *file_value;
   int indexValue;
   FcChar8 *foundry_value;
   FcChar8 *style_value;
   FcBool scalable;
   FcBool antialias;

   if (FcPatternGetString(pattern, FC_FAMILY, 0, &value) != FcResultMatch) {
      return;
   }

   familyName = QString::fromUtf8((const char *)value);

   slant_value   = FC_SLANT_ROMAN;
   weight_value  = FC_WEIGHT_REGULAR;
   spacing_value = FC_PROPORTIONAL;
   file_value    = nullptr;
   indexValue    = 0;
   scalable      = FcTrue;

   if (FcPatternGetInteger(pattern, FC_SLANT, 0, &slant_value) != FcResultMatch) {
      slant_value = FC_SLANT_ROMAN;
   }
   if (FcPatternGetInteger(pattern, FC_WEIGHT, 0, &weight_value) != FcResultMatch) {
      weight_value = FC_WEIGHT_REGULAR;
   }
   if (FcPatternGetInteger(pattern, FC_WIDTH, 0, &width_value) != FcResultMatch) {
      width_value = FC_WIDTH_NORMAL;
   }
   if (FcPatternGetInteger(pattern, FC_SPACING, 0, &spacing_value) != FcResultMatch) {
      spacing_value = FC_PROPORTIONAL;
   }
   if (FcPatternGetString(pattern, FC_FILE, 0, &file_value) != FcResultMatch) {
      file_value = nullptr;
   }

   if (FcPatternGetInteger(pattern, FC_INDEX, 0, &indexValue) != FcResultMatch) {
      indexValue = 0;
   }

   if (FcPatternGetBool(pattern, FC_SCALABLE, 0, &scalable) != FcResultMatch) {
      scalable = FcTrue;
   }

   if (FcPatternGetString(pattern, FC_FOUNDRY, 0, &foundry_value) != FcResultMatch) {
      foundry_value = nullptr;
   }

   if (FcPatternGetString(pattern, FC_STYLE, 0, &style_value) != FcResultMatch) {
      style_value = nullptr;
   }

   if (FcPatternGetBool(pattern, FC_ANTIALIAS, 0, &antialias) != FcResultMatch) {
      antialias = true;
   }

   QSupportedWritingSystems writingSystems;
   FcLangSet *langset = nullptr;
   FcResult res = FcPatternGetLangSet(pattern, FC_LANG, 0, &langset);

   if (res == FcResultMatch) {
      bool hasLang = false;

#if FC_VERSION >= 20297
      FcChar8 *cap = nullptr;
      FcResult capRes = FcResultNoMatch;
#endif
      for (int j = 1; j < QFontDatabase::WritingSystemsCount; ++j) {
         const FcChar8 *lang = (const FcChar8 *) languageForWritingSystem[j];

         if (lang) {
            FcLangResult langRes = FcLangSetHasLang(langset, lang);

            if (langRes != FcLangDifferentLang) {
#if FC_VERSION >= 20297
               if (*capabilityForWritingSystem[j] && requiresOpenType(j)) {

                  if (cap == nullptr) {
                     capRes = FcPatternGetString(pattern, FC_CAPABILITY, 0, &cap);
                  }

                  if (capRes == FcResultMatch && strstr(reinterpret_cast<const char *>(cap), capabilityForWritingSystem[j]) == nullptr) {
                     continue;
                  }
               }
#endif
               writingSystems.setSupported(QFontDatabase::WritingSystem(j));
               hasLang = true;
            }
         }
      }

      if (! hasLang) {
         // none of our known languages, add it to the other set
         writingSystems.setSupported(QFontDatabase::Other);
      }


   } else {
      // we set Other to supported for symbol fonts. It makes no
      // sense to merge these with other ones, as they are
      // special in a way.
      writingSystems.setSupported(QFontDatabase::Other);
   }

   FontFile *fontFile = new FontFile;
   fontFile->fileName = QString::fromUtf8((const char *)file_value);
   fontFile->indexValue = indexValue;

   QFont::Style style = (slant_value == FC_SLANT_ITALIC)
      ? QFont::StyleItalic : ((slant_value == FC_SLANT_OBLIQUE)
         ? QFont::StyleOblique : QFont::StyleNormal);

   // Note, weight should really be an int but registerFont incorrectly uses an enum
   QFont::Weight weight = QFont::Weight(weightFromFcWeight(weight_value));

   double pixel_size = 0;

   if (! scalable) {
      FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &pixel_size);
   }

   bool fixedPitch = spacing_value >= FC_MONO;

   // Note, stretch should really be an int but registerFont incorrectly uses an enum
   QFont::Stretch stretch = QFont::Stretch(stretchFromFcWidth(width_value));
   QString styleName = style_value ? QString::fromUtf8((const char *) style_value) : QString();

   QPlatformFontDatabase::registerFont(familyName, styleName, QString::fromUtf8((const char *)foundry_value), weight, style,
      stretch, antialias, scalable, pixel_size, fixedPitch, writingSystems, fontFile);

   for (int k = 1; FcPatternGetString(pattern, FC_FAMILY, k, &value) == FcResultMatch; ++k) {
      QPlatformFontDatabase::registerAliasToFontFamily(familyName, QString::fromUtf8((const char *)value));
   }
}

void QFontconfigDatabase::populateFontDatabase()
{
   FcInitReinitialize();
   FcFontSet  *fonts;

   {
      FcObjectSet *os = FcObjectSetCreate();
      FcPattern *pattern = FcPatternCreate();

      const char *properties [] = {
         FC_FAMILY, FC_STYLE, FC_WEIGHT, FC_SLANT,
         FC_SPACING, FC_FILE, FC_INDEX,
         FC_LANG, FC_CHARSET, FC_FOUNDRY, FC_SCALABLE, FC_PIXEL_SIZE,
         FC_WIDTH,
#if FC_VERSION >= 20297
         FC_CAPABILITY,
#endif
         (const char *)nullptr
      };

      const char **p = properties;

      while (*p) {
         FcObjectSetAdd(os, *p);
         ++p;
      }

      fonts = FcFontList(nullptr, pattern, os);
      FcObjectSetDestroy(os);
      FcPatternDestroy(pattern);
   }

   for (int i = 0; i < fonts->nfont; i++) {
      populateFromPattern(fonts->fonts[i]);
   }

   FcFontSetDestroy (fonts);

   struct FcDefaultFont {
      const char *qtname;
      const char *rawname;
      bool fixed;
   };

   const FcDefaultFont defaults[] = {
      { "Serif", "serif", false },
      { "Sans Serif", "sans-serif", false },
      { "Monospace", "monospace", true },
      { nullptr, nullptr, false }
   };

   const FcDefaultFont *f = defaults;

   // aliases only make sense for 'common', not for any of the specials
   QSupportedWritingSystems ws;
   ws.setSupported(QFontDatabase::Latin);

   while (f->qtname) {
      QString familyQtName = QString::fromLatin1(f->qtname);
      registerFont(familyQtName, QString(), QString(), QFont::Normal, QFont::StyleNormal, QFont::Unstretched,
                  true, true, 0, f->fixed, ws, nullptr);

      registerFont(familyQtName, QString(), QString(), QFont::Normal, QFont::StyleItalic, QFont::Unstretched,
                  true, true, 0, f->fixed, ws, nullptr);

      registerFont(familyQtName, QString(), QString(), QFont::Normal, QFont::StyleOblique, QFont::Unstretched,
                  true, true, 0, f->fixed, ws, nullptr);
      ++f;
   }

   // orignially it was a lazy population of the font db. want it to be initialized when
   // QApplication is constructed, so the population procedure can do something like this to
   // set the default font
   //    const FcDefaultFont *s = defaults;
   //    QFont font("Sans Serif");
   //    font.setPointSize(9);
   //    QApplication::setFont(font);
}

QFontEngineMulti *QFontconfigDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
   return new QFontEngineMultiFontConfig(fontEngine, script);
}

namespace {
QFontEngine::HintStyle defaultHintStyleFromMatch(QFont::HintingPreference hintingPreference, FcPattern *match, bool useXftConf)
{
   switch (hintingPreference) {
      case QFont::PreferNoHinting:
         return QFontEngine::HintNone;

      case QFont::PreferVerticalHinting:
         return QFontEngine::HintLight;

      case QFont::PreferFullHinting:
         return QFontEngine::HintFull;

      case QFont::PreferDefaultHinting:
         break;
   }

   if (QHighDpiScaling::isActive()) {
      return QFontEngine::HintNone;
   }

   int hint_style = 0;
   if (FcPatternGetInteger (match, FC_HINT_STYLE, 0, &hint_style) == FcResultMatch) {
      switch (hint_style) {
         case FC_HINT_NONE:
            return QFontEngine::HintNone;

         case FC_HINT_SLIGHT:
            return QFontEngine::HintLight;

         case FC_HINT_MEDIUM:
            return QFontEngine::HintMedium;

         case FC_HINT_FULL:
            return QFontEngine::HintFull;

         default:
            // emerald - should not happen, may want to throw
            break;
      }
   }

   if (useXftConf) {
      void *hintStyleResource = QGuiApplication::platformNativeInterface()->nativeResourceForScreen("hintstyle",
            QGuiApplication::primaryScreen());

      int hintStyle = int(reinterpret_cast<qintptr>(hintStyleResource));
      if (hintStyle > 0) {
         return QFontEngine::HintStyle(hintStyle - 1);
      }
   }

   return QFontEngine::HintFull;
}

QFontEngine::SubpixelAntialiasingType subpixelTypeFromMatch(FcPattern *match, bool useXftConf)
{
   int subpixel = FC_RGBA_UNKNOWN;

   if (FcPatternGetInteger(match, FC_RGBA, 0, &subpixel) == FcResultMatch) {
      switch (subpixel) {
         case FC_RGBA_UNKNOWN:
         case FC_RGBA_NONE:
            return QFontEngine::Subpixel_None;

         case FC_RGBA_RGB:
            return QFontEngine::Subpixel_RGB;

         case FC_RGBA_BGR:
            return QFontEngine::Subpixel_BGR;

         case FC_RGBA_VRGB:
            return QFontEngine::Subpixel_VRGB;

         case FC_RGBA_VBGR:
            return QFontEngine::Subpixel_VBGR;

         default:
            // emerald - should not happen, may want to throw
            break;
      }
   }

   if (useXftConf) {
      void *subpixelTypeResource = QGuiApplication::platformNativeInterface()->nativeResourceForScreen("subpixeltype",
            QGuiApplication::primaryScreen());

      int subpixelType = int(reinterpret_cast<qintptr>(subpixelTypeResource));

      if (subpixelType > 0) {
         return QFontEngine::SubpixelAntialiasingType(subpixelType - 1);
      }
   }

   return QFontEngine::Subpixel_None;
}
} // namespace

QFontEngine *QFontconfigDatabase::fontEngine(const QFontDef &f, void *usrPtr)
{
   if (! usrPtr) {
      return nullptr;
   }

   FontFile *fontfile = static_cast<FontFile *> (usrPtr);
   QFontEngine::FaceId fid;

   fid.filename = fontfile->fileName;
   fid.index    = fontfile->indexValue;

   QFontEngineFT *engine = new QFontEngineFT(f);
   engine->face_id = fid;

   setupFontEngine(engine, f);

   if (! engine->init(fid, engine->m_antialias, engine->defaultFormat) || engine->invalid()) {
      delete engine;
      engine = nullptr;
   }

   return engine;
}

QFontEngine *QFontconfigDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
   QFontEngineFT *engine = static_cast<QFontEngineFT *>(QBasicFontDatabase::fontEngine(fontData, pixelSize, hintingPreference));
   if (engine == nullptr) {
      return engine;
   }

   setupFontEngine(engine, engine->m_fontDef);

   return engine;
}

QStringList QFontconfigDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint,
   QChar::Script script) const
{
   QStringList fallbackFamilies;
   FcPattern *pattern = FcPatternCreate();

   if (! pattern) {
      return fallbackFamilies;
   }

   FcValue value;
   value.type = FcTypeString;

   QByteArray cs = family.toUtf8();
   value.u.s = (const FcChar8 *)cs.data();
   FcPatternAdd(pattern, FC_FAMILY, value, true);

   int slant_value = FC_SLANT_ROMAN;
   if (style == QFont::StyleItalic) {
      slant_value = FC_SLANT_ITALIC;
   } else if (style == QFont::StyleOblique) {
      slant_value = FC_SLANT_OBLIQUE;
   }
   FcPatternAddInteger(pattern, FC_SLANT, slant_value);

   Q_ASSERT(uint(script) < QChar::ScriptCount);

   if (*specialLanguages[script] != '\0') {
      FcLangSet *ls = FcLangSetCreate();
      FcLangSetAdd(ls, (const FcChar8 *)specialLanguages[script]);
      FcPatternAddLangSet(pattern, FC_LANG, ls);
      FcLangSetDestroy(ls);

   } else if (! family.isEmpty()) {
      // If script is Common or Han, then it may include languages like CJK,
      // we should attach system default language set to the pattern
      // to obtain correct font fallback list (i.e. if LANG=zh_CN
      // then we normally want to use a Chinese font for CJK text;
      // while a Japanese font should be used for that if LANG=ja)

      FcPattern *dummy = FcPatternCreate();
      FcDefaultSubstitute(dummy);
      FcChar8 *lang = nullptr;
      FcResult res = FcPatternGetString(dummy, FC_LANG, 0, &lang);

      if (res == FcResultMatch) {
         FcPatternAddString(pattern, FC_LANG, lang);
      }
      FcPatternDestroy(dummy);
   }

   const char *stylehint = getFcFamilyForStyleHint(styleHint);
   if (stylehint) {
      value.u.s = (const FcChar8 *)stylehint;
      FcPatternAddWeak(pattern, FC_FAMILY, value, FcTrue);
   }

   FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
   FcDefaultSubstitute(pattern);

   FcResult result    = FcResultMatch;
   FcFontSet *fontSet = FcFontSort(nullptr, pattern, FcFalse, nullptr, &result);
   FcPatternDestroy(pattern);

   if (fontSet) {
      QSet<QString> duplicates;
      duplicates.reserve(fontSet->nfont + 1);
      duplicates.insert(family.toCaseFolded());

      for (int i = 0; i < fontSet->nfont; i++) {
         FcChar8 *value = nullptr;

         if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &value) != FcResultMatch) {
            continue;
         }

         //         capitalize(value);
         const QString familyName = QString::fromUtf8((const char *)value);
         const QString familyNameCF = familyName.toCaseFolded();
         if (!duplicates.contains(familyNameCF)) {
            fallbackFamilies << familyName;
            duplicates.insert(familyNameCF);
         }
      }
      FcFontSetDestroy(fontSet);
   }

   return fallbackFamilies;
}

static FcPattern *queryFont(const FcChar8 *file, const QByteArray &data, int id, FcBlanks *blanks, int *count)
{
#if FC_VERSION < 20402
   return FcFreeTypeQuery(file, id, blanks, count);

#else
   if (data.isEmpty()) {
      return FcFreeTypeQuery(file, id, blanks, count);
   }

   FT_Library lib = qt_getFreetype();

   FcPattern *pattern = nullptr;

   FT_Face face;
   if (! FT_New_Memory_Face(lib, (const FT_Byte *)data.constData(), data.size(), id, &face)) {
      *count = face->num_faces;

      pattern = FcFreeTypeQueryFace(face, file, id, blanks);

      FT_Done_Face(face);
   }

   return pattern;
#endif
}

QStringList QFontconfigDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
   QStringList families;

   FcFontSet *set = FcConfigGetFonts(nullptr, FcSetApplication);

   if (! set) {
      FcConfigAppFontAddFile(nullptr, (const FcChar8 *)":/non-existent");
      set = FcConfigGetFonts(nullptr, FcSetApplication); // try again

      if (!set) {
         return families;
      }
   }

   FcBlanks *blanks = FcConfigGetBlanks(nullptr);

   int id    = 0;
   int count = 0;

   FcPattern *pattern;
   do {
      pattern = queryFont((const FcChar8 *)QFile::encodeName(fileName).constData(),
            fontData, id, blanks, &count);
      if (! pattern) {
         return families;
      }

      FcChar8 *fam = nullptr;
      if (FcPatternGetString(pattern, FC_FAMILY, 0, &fam) == FcResultMatch) {
         QString family = QString::fromUtf8(reinterpret_cast<const char *>(fam));
         families << family;
      }
      populateFromPattern(pattern);

      FcFontSetAdd(set, pattern);

      ++id;
   } while (id < count);

   return families;
}

QString QFontconfigDatabase::resolveFontFamilyAlias(const QString &family) const
{
   QString resolved = QBasicFontDatabase::resolveFontFamilyAlias(family);
   if (!resolved.isEmpty() && resolved != family) {
      return resolved;
   }

   FcPattern *pattern = FcPatternCreate();
   if (!pattern) {
      return family;
   }

   if (! family.isEmpty()) {
      QByteArray cs = family.toUtf8();
      FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *) cs.constData());
   }

   FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
   FcDefaultSubstitute(pattern);

   FcChar8 *familyAfterSubstitution = nullptr;
   FcPatternGetString(pattern, FC_FAMILY, 0, &familyAfterSubstitution);
   resolved = QString::fromUtf8((const char *) familyAfterSubstitution);
   FcPatternDestroy(pattern);

   return resolved;
}

QFont QFontconfigDatabase::defaultFont() const
{
   // Hack to get system default language until FcGetDefaultLangs()
   // is exported (https://bugs.freedesktop.org/show_bug.cgi?id=32853)
   // or https://bugs.freedesktop.org/show_bug.cgi?id=35482 is fixed
   FcPattern *dummy = FcPatternCreate();
   FcDefaultSubstitute(dummy);
   FcChar8 *lang = nullptr;
   FcResult res  = FcPatternGetString(dummy, FC_LANG, 0, &lang);

   FcPattern *pattern = FcPatternCreate();

   if (res == FcResultMatch) {
      // Make defaultFont pattern matching locale language aware, because
      // certain FC_LANG based custom rules may happen in FcConfigSubstitute()
      FcPatternAddString(pattern, FC_LANG, lang);
   }

   FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
   FcDefaultSubstitute(pattern);

   FcChar8 *familyAfterSubstitution = nullptr;
   FcPatternGetString(pattern, FC_FAMILY, 0, &familyAfterSubstitution);

   QString resolved = QString::fromUtf8((const char *) familyAfterSubstitution);
   FcPatternDestroy(pattern);
   FcPatternDestroy(dummy);

   return QFont(resolved);
}

void QFontconfigDatabase::setupFontEngine(QFontEngineFT *engine, const QFontDef &fontDef) const
{
   bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
   bool forcedAntialiasSetting = !antialias;

   const QPlatformServices *services = QGuiApplicationPrivate::platformIntegration()->services();
   bool useXftConf = (services && (services->desktopEnvironment() == "GNOME" || services->desktopEnvironment() == "UNITY"));

   if (useXftConf && !forcedAntialiasSetting) {
      void *antialiasResource = QGuiApplication::platformNativeInterface()->
                  nativeResourceForScreen("antialiasingEnabled", QGuiApplication::primaryScreen());

      int antialiasingEnabled = int(reinterpret_cast<qintptr>(antialiasResource));
      if (antialiasingEnabled > 0) {
         antialias = antialiasingEnabled - 1;
      }
   }

   QFontEngine::GlyphFormat format;
   // try and get the pattern
   FcPattern *pattern = FcPatternCreate();

   FcValue value;
   value.type = FcTypeString;

   QByteArray cs = fontDef.family.toUtf8();
   value.u.s = (const FcChar8 *)cs.data();
   FcPatternAdd(pattern, FC_FAMILY, value, true);

   QFontEngine::FaceId fid = engine->faceId();

   if (! fid.filename.isEmpty()) {
      value.u.s = (const FcChar8 *)fid.filename.data();
      FcPatternAdd(pattern, FC_FILE, value, true);

      value.type = FcTypeInteger;
      value.u.i = fid.index;
      FcPatternAdd(pattern, FC_INDEX, value, true);
   }

   if (fontDef.pixelSize > 0.1) {
      FcPatternAddDouble(pattern, FC_PIXEL_SIZE, fontDef.pixelSize);
   }

   FcResult result;

   FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
   FcDefaultSubstitute(pattern);

   FcPattern *match = FcFontMatch(nullptr, pattern, &result);
   if (match) {
      engine->setDefaultHintStyle(defaultHintStyleFromMatch((QFont::HintingPreference)fontDef.hintingPreference, match, useXftConf));

      FcBool fc_autohint;
      if (FcPatternGetBool(match, FC_AUTOHINT, 0, &fc_autohint) == FcResultMatch) {
         engine->forceAutoHint = fc_autohint;
      }

#if defined(FT_LCD_FILTER_H)
      int lcdFilter;
      if (FcPatternGetInteger(match, FC_LCD_FILTER, 0, &lcdFilter) == FcResultMatch) {
         engine->lcdFilterType = lcdFilter;
      }
#endif

      if (!forcedAntialiasSetting) {
         FcBool fc_antialias;
         if (FcPatternGetBool(match, FC_ANTIALIAS, 0, &fc_antialias) == FcResultMatch) {
            antialias = fc_antialias;
         }
      }

      if (antialias) {
         QFontEngine::SubpixelAntialiasingType subpixelType = QFontEngine::Subpixel_None;
         if (!(fontDef.styleStrategy & QFont::NoSubpixelAntialias)) {
            subpixelType = subpixelTypeFromMatch(match, useXftConf);
         }
         engine->subpixelType = subpixelType;

         format = (subpixelType == QFontEngine::Subpixel_None)
            ? QFontEngine::Format_A8 : QFontEngine::Format_A32;
      } else {
         format = QFontEngine::Format_Mono;
      }

      FcPatternDestroy(match);
   } else {
      format = antialias ? QFontEngine::Format_A8 : QFontEngine::Format_Mono;
   }

   FcPatternDestroy(pattern);

   engine->m_antialias   = antialias;
   engine->defaultFormat = format;
   engine->glyphFormat   = format;
}



﻿#ifndef VoiceWAV_h
#define VoiceWAV_h

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include "Voice.h"
#include "../core/core-inl.h"
#include "../core/PitchMarker.h"
#include "../core/UnitWaveformMaker.h"
#include "../format/PitchmarkParameters.h"
#include "../format/PronunciationAlias.h"
#include "../format/UnitWaveformContainer.h"
#include "../parser/WavHandler.h"
#include "../parser/WavParser.h"

class VoiceWAV: public Voice, public WavHandler {
 public:
  static struct Parameters {
    Parameters() {
      use_pmp_cache = true;
      make_uwc_cache = false;
      make_pmp_cache = false;
    }
    bool use_pmp_cache;
    bool make_uwc_cache;
    bool make_pmp_cache;
  } params;

  static struct PitchmarkTemplateParameters {
    std::wstring filename;
    std::vector<double> waveform;
    long from;
    long to;
  };

  VoiceWAV(const std::wstring& str_pron_alias, const boost::filesystem::path& path):Voice(str_pron_alias, path){}
  VoiceWAV(const PronunciationAlias& pron_alias, const boost::filesystem::path& path):Voice(pron_alias, path){}
  VoiceWAV(const VoiceWAV& other):Voice(other){}
  ~VoiceWAV(){}

  const UnitWaveformContainer* getUnitWaveformContainer() const;
  void makeUnitWaveformContainerCache(bool save_memory_cache=true) const;

 private:
  mutable Wav tmp_wav;
  mutable struct Vowel {
    std::vector<double> wav;
    long ms_from = 0;
  } fore_vowel, aft_vowel;

  Vowel getVowelWav(std::vector<double>::const_iterator base_pitchmark) const;
  void setVowelWav();
  void setPrefixVowelWav();
  inline double sinc(double x) const { return sin(M_PI*x) / (M_PI*x); }

  // inherit from WavHandler
  void chunkHeader(WavHeader wav_header);
  void chunkData(WavData wav_data);
};

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#endif

#ifndef PitchArranger_h
#define PitchArranger_h

#include <cmath>
#include <vector>
#include "Arranger.h"
#include "../Score.h"
#include "../parser/WavFormat.h"

class PitchArranger : Arranger {
 public:
  const static short overshoot_length;
  const static double overshoot_height;
  const static short preparation_length;
  const static double preparation_height;
  const static short vibrato_offset;
  const static short vibrato_width;
  const static double vibrato_depth;

  static void arrange(Score *score, WavFormat format);

 private:
  static void vibrato(std::vector<double> *guide_pitches);
  static void overshoot(std::vector<double> *guide_pitches, double pitch_from, double pitch_to);
  static void preparation(std::vector<double> *guide_pitches, double pitch_from, double pitch_to);
};

#endif

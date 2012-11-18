#include "BaseWavsOverlapper.h"

using namespace std;

BaseWavsOverlapper::BaseWavsOverlapper():rep_start(-1), velocity(1.0){}

BaseWavsOverlapper::~BaseWavsOverlapper(){}

bool BaseWavsOverlapper::overlapping()
{
  if (pitch_marks.empty() || base_wavs.empty() || rep_start<0)
    return false;
  cout << "----- start overlapping -----" << endl;

  output_wav.clear();
  output_wav.assign(pitch_marks.back(), 0);
  vector<long> tmp_output_wav(output_wav.size(), 0);
  long fade_start = base_wavs[rep_start+((base_wavs.size()-rep_start)/2)].fact.dwPosition;
  long fade_last = base_wavs.back().fact.dwPosition;
  vector<BaseWav>::iterator tmp_base_wav = base_wavs.begin();
  cout << "output size:" << output_wav.size() << endl;
  cout << "fade_start:" << fade_start << ", fade_last:" << fade_last << endl;

  for (int i=0; i<pitch_marks.size()-1; i++) {
    long tmp_dist, tmp_pitch_mark = (pitch_marks[i] > fade_last)
      ? fade_start + ((pitch_marks[i]-fade_last)%(fade_last-fade_start))
      : pitch_marks[i];
    long num_base_wav = 0;
    do {
      tmp_dist = abs(tmp_pitch_mark-(base_wavs[num_base_wav].fact.dwPosition));
      if (++num_base_wav == base_wavs.size()) {
        --num_base_wav;
        break;
      }
    } while (tmp_dist>abs(tmp_pitch_mark-(base_wavs[num_base_wav]).fact.dwPosition));
    BaseWav base_wav = base_wavs[--num_base_wav];
    vector<short> win = base_wav.data.getDataVector();
    long win_start = pitch_marks[i] - base_wav.fact.dwPitchLeft;
    long win_end = pitch_marks[i] + base_wav.fact.dwPitchRight;
    if (win_start < 0) {
      // left edge
      win.erase(win.begin(), win.begin()-win_start);
      win_start = 0;
    }
    if (win_end >= pitch_marks.back()) {
      // right edge
      win.erase(win.end()-(win_end-pitch_marks.back()), win.end());
      win_end = pitch_marks.back();
    }
    for (int j=0; j<win_end-win_start; j++)
      tmp_output_wav[win_start+j] += win[j];
  }

  // normalize
  double scale = max((*max_element(tmp_output_wav.begin(), tmp_output_wav.end()))/(double)numeric_limits<short>::max(),
    (*min_element(tmp_output_wav.begin(), tmp_output_wav.end()))/(double)numeric_limits<short>::min());
  scale = (scale>0.8)?(0.8/scale):1.0;
  for (int i=0; i<output_wav.size(); i++)
    output_wav[i] = tmp_output_wav[i] * scale * velocity;

  cout << "----- finish overlapping -----" << endl << endl;
  return true;
}

void BaseWavsOverlapper::debugTxt(string output)
{
  ofstream ofs(output.c_str());

  for (int i=0; i<output_wav.size(); i++)
    ofs << output_wav[i] << endl;
}

void BaseWavsOverlapper::debugWav(string output)
{
  cout << "start wav output" << endl;

  long size_all = 28 + (output_wav.size()*sizeof(short)+8);
  WavFormat format;
  format.setDefaultValues();
  ofstream ofs(output.c_str(), ios_base::out|ios_base::trunc|ios_base::binary);
  WavParser::setWavHeader(&ofs, format, size_all);

  // data chunk
  WavData data;
  data.setData(output_wav);
  long data_size = data.getSize(); 
  ofs.write((char*)WavFormat::data, sizeof(char)*4);
  ofs.write((char*)&data_size, sizeof(long));
  ofs.write((char*)data.getData(), data.getSize());

  cout << "finish wav output" << endl;
}

/*
 * accessor
 */
void BaseWavsOverlapper::setPitchMarks(list<unsigned long> pitch_marks)
{
  this->pitch_marks.clear();
  this->pitch_marks.assign(pitch_marks.begin(), pitch_marks.end());
}

void BaseWavsOverlapper::setPitchMarks(vector<unsigned long> pitch_marks)
{
  this->pitch_marks = pitch_marks;
}

list<unsigned long> BaseWavsOverlapper::getPitchMarkList()
{
  list<unsigned long> pitch_marks;

  for (int i=0; i<this->pitch_marks.size(); ++i)
    pitch_marks.push_back(this->pitch_marks[i]);

  return pitch_marks;
}

vector<unsigned long> BaseWavsOverlapper::getPitchMarkVector()
{
  return pitch_marks;
}

void BaseWavsOverlapper::setBaseWavs(vector<BaseWav> base_wavs)
{
  this->base_wavs = base_wavs;
}

vector<BaseWav> BaseWavsOverlapper::getBaseWavs()
{
  return base_wavs;
}

list<short> BaseWavsOverlapper::getOutputWavList()
{
  list<short> output_wav;

  for (int i=0; i<this->output_wav.size(); ++i)
    output_wav.push_back(this->output_wav[i]);

  return output_wav;
}

vector<short> BaseWavsOverlapper::getOutputWavVector()
{
  return output_wav;
}

void BaseWavsOverlapper::setRepStart(unsigned long rep_start)
{
  this->rep_start = rep_start;
}

unsigned long BaseWavsOverlapper::getRepStart()
{
  return rep_start;
}

void BaseWavsOverlapper::setVelocity(unsigned short velocity)
{
  this->velocity = velocity/100.0;
}

void BaseWavsOverlapper::setVelocity(double velocity)
{
  this->velocity = velocity;
}

double BaseWavsOverlapper::getVelocity()
{
  return velocity;
}

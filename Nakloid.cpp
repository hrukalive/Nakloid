#include "Nakloid.h"

using namespace std;

Nakloid::Nakloid() : voice_db(0), margin(0)
{
  setDefaultFormat();
}

Nakloid::Nakloid(string singer, string path_score, short track, string path_lyric) : voice_db(0), margin(0)
{
  setDefaultFormat();
  setSinger(singer);
  setScorePath(path_score, track, path_lyric);
}

Nakloid::~Nakloid()
{
  if (voice_db != 0) {
    delete voice_db;
    voice_db = 0;
  }
}

void Nakloid::setDefaultFormat()
{
  format.chunkSize = 16;
  format.wFormatTag = 1;
  format.wChannels = 1;
  format.dwSamplesPerSec = 44100;
  format.dwAvgBytesPerSec = format.dwSamplesPerSec*2;
  format.wBlockAlign = 2;
  format.wBitsPerSamples = 16;
}

bool Nakloid::setScorePath(string path_score, short track, string path_lyric)
{
  // set score
  Score *score = new Score(path_score, track);
  if (score == 0 || !score->isScoreLoaded()) {
    cerr << "score hasn't loaded" << endl;
    return false;
  }
  notes = score->getNotesVector();
  delete score;

  // set lyric
  ifstream ifs(path_lyric);
  string buf_str;
  list<string> buf_list;
  while (ifs && getline(ifs, buf_str)) {
    if (buf_str == "")
      continue;
    if (*(buf_str.end()-1) == ',')
      buf_str.erase(buf_str.end()-1,buf_str.end());
    vector<string> buf_vector;
    boost::algorithm::split(buf_vector, buf_str, boost::is_any_of(","));
    buf_list.insert(buf_list.end(), buf_vector.begin(), buf_vector.end());
  }
  list<string>::iterator it = buf_list.begin();
  for (int i=0; i<notes.size(); i++) {
    notes[i].setPron(*it);
    if (++it == buf_list.end())
      break;
  }

  return true;
}

bool Nakloid::vocalization()
{
  if (notes.empty()) {
    cerr << "note hasn't loaded" << endl;
    return false;
  }

  if (voice_db == 0) {
    cerr << "can't find voiceDB" << endl;
    return false;
  }

  if (path_song.empty()) {
    cerr << "song path is empty" << endl;
    return false;
  }

  cout << "----- start vocalization -----" << endl;

  double tmp_size = 0.0;
  long size;
  if (margin < voice_db->getVoice(notes[0].getPron()).prec)
    margin = voice_db->getVoice(notes[0].getPron()).prec;
  for (int i=0; i<notes.size(); i++)
    tmp_size += notes[i].getStart()+notes[i].getLength();
  size = ms2pos(tmp_size+margin) * sizeof(short);

  ofstream ofs(path_song.c_str(), ios_base::out|ios_base::trunc|ios_base::binary);
  WavParser::setWavHeader(&ofs, format, size+28);
  ofs.write((char*)WavFormat::data, sizeof(char)*4);
  ofs.write((char*)&size, sizeof(long));

  vector<short> output_wav(size/sizeof(short),0);
  long start_ms=0, end_ms=margin;
  for (int pos_note=0; pos_note<notes.size(); pos_note++) {
    Voice voice = voice_db->getVoice(notes[pos_note].getPron());
    Voice voice_prev = (pos_note<=0)?voice_db->getVoice(""):voice_db->getVoice(notes[pos_note-1].getPron());
    Voice voice_next = (pos_note+1>=notes.size())?voice_db->getVoice(""):voice_db->getVoice(notes[pos_note+1].getPron());
    cout << "pos_note:" << pos_note << ", file:" << voice.filename << endl;

    // set pos
    double pos_start, pos_end, note_len_ms=voice.prec+notes[pos_note].getLength()-(voice_next.prec-voice_next.ovrl);
    start_ms = end_ms + notes[pos_note].getStart();
    end_ms = start_ms + notes[pos_note].getLength();
    pos_start = ms2pos(start_ms-voice.prec);
    pos_end = ms2pos(end_ms-(voice_next.prec-voice_next.ovrl));
    if (pos_end >= output_wav.size())
      pos_end = output_wav.size() - 1;

    // make guide pitch
    double pitch = notes[pos_note].getPitchHz();
    vector<double> guide_pitches((long)note_len_ms, pitch);

    // arrange pitch mark
    PitchArranger::vibrato(&guide_pitches);
    //if (pos_note>0 && notes[pos_note].getStart()==0)
      //PitchArranger::overshoot(&guide_pitches, notes[pos_note-1].getPitchHz(), notes[pos_note].getPitchHz());
    if (pos_note<notes.size()-1 && notes[pos_note+1].getStart()==0)
      PitchArranger::preparation(&guide_pitches, notes[pos_note].getPitchHz(), notes[pos_note+1].getPitchHz());

    // guide_pitches to pitch_marks
    vector<long> voice_marks(note_len_ms/1000.0*pitch+1, 0);
    double tmp_mark=0.0, pos_guide_pitches=0.0;
    for (int i=1; i<(int)voice_marks.size(); i++) {
      tmp_mark += format.dwSamplesPerSec/(double)guide_pitches[(long)pos_guide_pitches];
      voice_marks[i] = (long)tmp_mark;
      pos_guide_pitches += 1000.0 / guide_pitches[(long)pos_guide_pitches];
      if (pos_guide_pitches >= guide_pitches.size())
        pos_guide_pitches = guide_pitches.size() - 1;
    }
    long back_mark = pos_end - pos_start;
    if (voice_marks.back() != back_mark) {
      while (voice_marks.back() > back_mark)
        voice_marks.erase(voice_marks.end()-1);
      voice_marks.push_back(back_mark);
    }

    // make note wav
    BaseWavsOverlapper *overlapper = new BaseWavsOverlapper();
    overlapper->setPitchMarks(voice_marks);
    overlapper->setBaseWavs(voice.bwc.data);
    overlapper->setRepStart(voice.bwc.format.dwRepeatStart);
    overlapper->overlapping();
    vector<short> note_wav = overlapper->getOutputWavVector();
    delete overlapper;

    // smooth note edge
    if (notes[pos_note].getStart() == 0)
      DataArranger::edge_front(&note_wav, format.dwSamplesPerSec);
    DataArranger::edge_back(&note_wav, format.dwSamplesPerSec);

    // set output
    for (int i=0; i<(int)note_wav.size(); i++)
      output_wav[pos_start+i] += note_wav[i];
  }

  ofs.write((char*)(&output_wav[0]), size);
  ofs.close();

  cout << "----- vocalization finished -----" << endl << endl;
  return true;
}

bool Nakloid::vocalization(string path_song)
{
  setSongPath(path_song);
  return vocalization();
}

WavFormat Nakloid::getFormat()
{
  return format;
}

void Nakloid::setFormat(WavFormat format)
{
  this->format = format;
}

vector<Note> Nakloid::getNotes()
{
  return notes;
}

void Nakloid::setSongPath(string path_song)
{
  this->path_song = path_song;
}

string Nakloid::getSongPath()
{
  return path_song;
}

void Nakloid::setSinger(string singer)
{
  if (voice_db != 0) {
    delete voice_db;
    voice_db = 0;
  }
  voice_db = new VoiceDB(singer);
}

string Nakloid::getSinger()
{
  return voice_db->getSinger();
}

void Nakloid::setMargin(long margin)
{
  this->margin = margin;
}

long Nakloid::getMargin()
{
  return this->margin;
}

double Nakloid::ms2pos(long ms)
{
  return ms / 1000.0 * format.dwSamplesPerSec;
}
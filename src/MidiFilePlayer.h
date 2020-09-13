#ifndef MIDIFILEPLAYER_H
#define MIDIFILEPLAYER_H

#include "Libraries.h"
#include "Transport.h"
#include "MidiPlaybackThread.h"

class MidiFilePlayer : public Component,
                       public ComboBoxListener,
                       public MidiPlaybackThread::MidiMessageSource,
                       public Transport::Listener
{

public:

  MidiFilePlayer();
  ~MidiFilePlayer();

  void setFile(juce::File file=juce::File::nonexistent);
  const juce::File& getFile();
  static void openMidiFilePlayer(juce::File midiFile=juce::File::nonexistent);

private:
  // Transport Callbacks
  void play(double position);
  void pause();
  void positionChanged(double position, bool isPlaying, int dir);
  void tempoChanged(double tempo, bool isPlaying);
  // MidiPlaybackThread callback
  void addMidiPlaybackMessages(MidiPlaybackThread::MidiMessageQueue& queue, 
                               MidiPlaybackThread::PlaybackPosition& position);
  // Gui Callbacks
  void resized();
  void comboBoxChanged (juce::ComboBox* combobox);
 
  //==============================================================================
  juce::File midiFile;
  int fileLength;
  double fileDuration;
  juce::MidiMessageSequence sequence;
  MidiPlaybackThread* pbthread;
  Transport* transport;

  juce::Label* fileInfo;
  juce::Label* midiPortLabel;
  juce::ComboBox* midiPortMenu;
  juce::MidiOutput* midiPort;
  //==============================================================================
  // (prevent copy constructor and operator= being generated..)
  MidiFilePlayer (const MidiFilePlayer&);
  const MidiFilePlayer& operator= (const MidiFilePlayer&);
};

#endif





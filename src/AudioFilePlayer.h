#ifndef AUDIOFILEPLAYER_H
#define AUDIOFILEPLAYER_H

#include "Libraries.h"
#include "Transport.h"

class AudioFilePlayer : public Component,
                        public SliderListener,
                        public Transport::Listener,
                        // needed to move transport's position slider
                        // during playback
                        public AudioIODeviceCallback
                        
{
public:
  AudioFilePlayer (AudioDeviceManager& deviceManager_);
  ~AudioFilePlayer();

  static void openAudioFilePlayer(juce::File fileToOpen, bool play);
  void setFile (const File& file);
  const File& getFile();

  // Transport Callbacks
  void play(double position);
  void pause();
  void positionChanged(double position, bool isPlaying, int direction);
  void tempoChanged(double tempo, bool isPlaying);

  // Wrappers to move transport's position slider during playback
  void audioDeviceIOCallback (const float** inputChannelData,
                              int totalNumInputChannels,
                              float** outputChannelData,
                              int totalNumOutputChannels,
                              int numSamples);
  void audioDeviceAboutToStart (AudioIODevice* device);
  void audioDeviceStopped();

  // GUI Callbacks
  void resized();
  void sliderValueChanged (Slider* sliderThatWasMoved);

  //==============================================================================
  juce_UseDebuggingNewOperator

  private:
  AudioDeviceManager& deviceManager;
  AudioSourcePlayer audioSourcePlayer;
  AudioTransportSource transportSource;
  ScopedPointer<AudioFormatReaderSource> currentAudioFileSource;
  ScopedPointer<ResamplingAudioSource> resamplingSource;

  void loadFileIntoTransport (const File& audioFile);
  void showFileInfo ();

  //==============================================================================
  File audioFile;
  Label* gainLabel;
  Label* fileInfo;
  Slider* gainSlider;
  Transport* transport;
  double fileDuration;

  //==============================================================================
  // (prevent copy constructor and operator= being generated..)
  AudioFilePlayer (const AudioFilePlayer&);
  const AudioFilePlayer& operator= (const AudioFilePlayer&);
};

#endif


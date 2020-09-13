#include "Enumerations.h"
#include "AudioFilePlayer.h"
#include "Transport.h"
#include "Audio.h"

//==============================================================================
AudioFilePlayer::AudioFilePlayer (AudioDeviceManager& deviceManager_)
    : deviceManager (deviceManager_),
      audioFile (File::nonexistent),
      gainLabel (0),
      gainSlider (0),
      transport(0),
      fileDuration(0.0)
{
  Transport::TempoConfig config (1.0, 0.25, 8.0, 0.0, 1.0, juce::String(" Srate"), 3);

  addAndMakeVisible(transport = new Transport(this,&config,true));
  addAndMakeVisible(fileInfo = new Label());
  fileInfo->setJustificationType(Justification(Justification::horizontallyCentred));
  fileInfo->setFont(Font::Font(14.0));
  fileInfo->setColour(Label::textColourId, Colours::darkgrey);
  setSize (400, 8+24+8+Transport::TransportHeight+8);

  deviceManager.addAudioCallback (this);
  ////  deviceManager.addAudioCallback (&audioSourcePlayer);
  resamplingSource = new ResamplingAudioSource(&transportSource, false, 2);
  audioSourcePlayer.setSource (resamplingSource); //&transportSource
}

//==============================================================================
AudioFilePlayer::~AudioFilePlayer()
{
  transportSource.setSource (0);
  audioSourcePlayer.setSource (0);
  resamplingSource = 0;
  deviceManager.removeAudioCallback (this);
  ////  deviceManager.removeAudioCallback (&audioSourcePlayer);
  deleteAndZero (transport);
  deleteAndZero (fileInfo);
}

//==============================================================================
void AudioFilePlayer::setFile (const File& file)
{
  std::cout << "setFile(" << file.getFullPathName() << ")\n";
  audioFile=file;
  loadFileIntoTransport (file);
  //  gainSlider->setValue (0, false, false);
  transport->setPausing(false);
  transport->setPlaybackPosition(0, false);
  // setSrate(1.0);
  showFileInfo();
}

const File& AudioFilePlayer::getFile()
{
  return audioFile;
}

//==============================================================================
// GUI Callbacks
//==============================================================================

void AudioFilePlayer::resized()
{
  fileInfo->setBounds(8, 8, getWidth()-16, 24);
  transport->setBounds (8, 32, getWidth()-6, Transport::TransportHeight);
  //    gainSlider->setBounds (72, getHeight() - 90, 200, 24);
}

void AudioFilePlayer::showFileInfo()
{
  juce::String info (juce::String::empty);
  if (currentAudioFileSource)
  {
    if (AudioFormatReader* reader=currentAudioFileSource->getAudioFormatReader())
    {
      info << audioFile.getFileName() << ":" //<< ": format=" << reader->getFormatName()
           << " chans=" << (int)reader->numChannels
           << ", sr=" << reader->sampleRate
           << ", dur=" << ((double)(reader->lengthInSamples/reader->sampleRate));
    }
  }
  fileInfo->setText(info, false);
}

void AudioFilePlayer::sliderValueChanged (Slider* sliderThatWasMoved)
{
  if (sliderThatWasMoved == gainSlider)
  {
  }
}

//==============================================================================
// Transport Callbacks
//==============================================================================

void AudioFilePlayer::play(double position)
{
  std::cout << "play(" << position << ")\n";
  transportSource.setPosition(fileDuration*position);
  transportSource.start();
}

void AudioFilePlayer::pause()
{
  std::cout << "pause()\n";
  transportSource.stop();
}

void AudioFilePlayer::positionChanged(double position, bool isPlaying, int direction)
{
  std::cout << "positionChanged(" << position << "," << isPlaying << "," << direction << ")\n";
  transportSource.setPosition(fileDuration*position);
}

void AudioFilePlayer::tempoChanged(double tempo, bool isPlaying)
{
  std::cout << "tempoChanged(" << tempo << "," << isPlaying << ")\n";
  resamplingSource->setResamplingRatio(tempo);
}

//==============================================================================
// Audio Support Routines
//==============================================================================

void AudioFilePlayer::loadFileIntoTransport (const File& audioFile)
{
  std::cout << "loadFileIntoTransport(" << audioFile.getFullPathName() << ")\n";
  // unload the previous file source and delete it..
  transportSource.stop();
  transportSource.setSource (0);

  currentAudioFileSource = 0;
  ////  currentResamplingAudioSource = 0;
  
  // get a format manager and set it up with the basic types (wav and aiff).
  AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  AudioFormatReader* reader = formatManager.createReaderFor (audioFile);
  fileDuration=0;
  if (reader != 0)
  {
    currentAudioFileSource = new AudioFormatReaderSource (reader, true);
    //    currentResamplingAudioSource = new ResamplingAudioSource(currentAudioFileSource, false, reader->numChannels);
    // ..and plug it into our transport source
    transportSource.setSource (currentAudioFileSource, ////currentResamplingAudioSource
                               32768, // tells it to buffer this many samples ahead
                               reader->sampleRate);
    fileDuration=(double)(reader->lengthInSamples/reader->sampleRate) ;
  }
}

/// audioDeviceIOCallback wrapper calls real source then moves the
/// transport's position slider

void AudioFilePlayer::audioDeviceIOCallback (const float** inputChannelData,
                                             int totalNumInputChannels,
                                             float** outputChannelData,
                                             int totalNumOutputChannels,
                                             int numSamples)
{
  // pass audio info on to our player source
  audioSourcePlayer.audioDeviceIOCallback(inputChannelData, 
					  totalNumInputChannels,
					  outputChannelData,
					  totalNumOutputChannels, 
					  numSamples);
  // update transport slider to make it move left to right during
  // playback. not straightforward because this callback is called
  // even if a file is not playing.

  static int counter=0; // counter for downsampling
  static bool wasStreamFinished = false;

  if (wasStreamFinished && !transportSource.hasStreamFinished())
  {
    wasStreamFinished = !wasStreamFinished;
  }
  else if (!wasStreamFinished && transportSource.hasStreamFinished())
  {
    // auto stop transport. we've reached the end while playing so
    // stop playback and move the position slider to the start.
    transport->sendMessage(Transport::SetPausing, 0, 0, true);
    transport->sendMessage(Transport::SetPlaybackPosition, 0, 0, false);
    wasStreamFinished = !wasStreamFinished;
  }

  // downsample to prevent updating the GUI at audio rate!
  if (counter==16)
  {
    if (transportSource.isPlaying())
    {
      double pos=transportSource.getCurrentPosition()/fileDuration;
      transport->sendMessage(Transport::SetPlaybackPosition, pos, 0, false);
    }
    counter=0;
  }
  else 
    counter++;
}

void AudioFilePlayer::audioDeviceAboutToStart (AudioIODevice* device)
{
  std::cout << "audioDeviceAboutToStart\n";
  audioSourcePlayer.audioDeviceAboutToStart(device);
}

void AudioFilePlayer::audioDeviceStopped()
{
  std::cout << "audioDeviceStopped\n";
  audioSourcePlayer.audioDeviceStopped();
}

//==============================================================================
// Audio Window
//==============================================================================

class AudioFilePlayerWindow : public DocumentWindow 
{
public:
  AudioFilePlayerWindow()
    : DocumentWindow(T("Audio File Player"), Colour(0xffe5e5e5), DocumentWindow::allButtons, true)
  {
    setUsingNativeTitleBar(true);
    setDropShadowEnabled(true);
    AudioFilePlayer* player=new AudioFilePlayer(*AudioManager::getInstance());
    player->setVisible(true);
    setContentOwned(player,true);
    setCentreRelative(0.5,0.5);
    setVisible(true);
    WindowTypes::setWindowType(this,WindowTypes::AudioFilePlayer);
  }
  ~AudioFilePlayerWindow() 
  {
  }
  AudioFilePlayer* getAudioFilePlayer()
  {
    return dynamic_cast<AudioFilePlayer*> (getContentComponent());
  }
  void closeButtonPressed()
  {
    delete this;
  }
};

void AudioFilePlayer::openAudioFilePlayer(juce::File fileToOpen, bool play)
{
  static File lastOpened=File::nonexistent;
  AudioFilePlayerWindow* win = NULL; 

  if (fileToOpen.existsAsFile())
  {
    // reuse a player if its already open for the file
    for (int i=0; i<juce::TopLevelWindow::getNumTopLevelWindows(); i++)
    {
      juce::TopLevelWindow* w=juce::TopLevelWindow::getTopLevelWindow(i);
      if (WindowTypes::isWindowType(w, WindowTypes::AudioFilePlayer))
      {
        win=dynamic_cast<AudioFilePlayerWindow*>(w);
        if (win->getAudioFilePlayer()->getFile()==fileToOpen)
          break;
        win=NULL;
      }
    }
  }
  else  // select file if one is not provided or is bogus
  {
    FileChooser choose ("Play Audio File", lastOpened, "*.wav;*.aiff;*.aif");
    if (choose.browseForFileToOpen())
    {
      fileToOpen=choose.getResult();
      lastOpened=fileToOpen;
    }
    else 
      return;
  }

  // create new player or raise old one to top of window stack
  if (win==NULL)
    win=new AudioFilePlayerWindow();
  else
    win->getTopLevelComponent()->toFront(true); 
  // set the player's file to our file
  win->getAudioFilePlayer()->setFile(fileToOpen);
}

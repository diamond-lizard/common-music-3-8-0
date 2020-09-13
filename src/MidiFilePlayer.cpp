#include "Enumerations.h"
#include "MidiFilePlayer.h"
#include "Midi.h"

//==============================================================================
// MidiFilePlayerWindow
//==============================================================================

class MidiFilePlayerWindow : public DocumentWindow 
{
public:
  MidiFilePlayerWindow ()
    : DocumentWindow(T("Midi File Player"), Colour(0xffe5e5e5), DocumentWindow::allButtons, true)
  {
    setUsingNativeTitleBar(true);
    setDropShadowEnabled(true);
    setContentOwned(new MidiFilePlayer(), true);
    setCentreRelative(0.5,0.5);
    setVisible(true);
    WindowTypes::setWindowType(this,WindowTypes::MidiFilePlayer);
  }
  ~MidiFilePlayerWindow()
  {
  }
  void closeButtonPressed()
  {
    delete this;
  }
  MidiFilePlayer* getMidiFilePlayer()
  {
    return dynamic_cast<MidiFilePlayer*>(getContentComponent());
  }
};

void MidiFilePlayer::openMidiFilePlayer(juce::File fileToOpen)
{
  static juce::File lastOpened=File::nonexistent;
  MidiFilePlayerWindow* win = NULL; 

  if (fileToOpen.existsAsFile())
  {
    // reuse a player if its already open for the file
    for (int i=0; i<juce::TopLevelWindow::getNumTopLevelWindows(); i++)
    {
      juce::TopLevelWindow* w=juce::TopLevelWindow::getTopLevelWindow(i);
      if (WindowTypes::isWindowType(w, WindowTypes::MidiFilePlayer))
      {
        win=dynamic_cast<MidiFilePlayerWindow*>(w);
        if (win->getMidiFilePlayer()->getFile()==fileToOpen)
          break;
        win=NULL;
      }
    }
  }
  else  // select file if one is not provided or is bogus
  {
    juce::FileChooser choose ("Play Midi File", lastOpened, "*.mid;*.midi");
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
    win=new MidiFilePlayerWindow();
  else
    win->getTopLevelComponent()->toFront(true); 
  // set the player's file to our file
  win->getMidiFilePlayer()->setFile(fileToOpen);
}

//================================================================================
// MidiFilePlayer
//==============================================================================

MidiFilePlayer::MidiFilePlayer()
  : midiFile (juce::File::nonexistent),
    fileLength (0),
    fileDuration (0.0),
    midiPort (0),
    fileInfo (0),
    midiPortLabel (0),
    midiPortMenu (0),
    transport (0)
{
  Transport::TempoConfig config (60.0);
  addAndMakeVisible(transport=new Transport(this, &config));

  addAndMakeVisible(fileInfo = new juce::Label());
  fileInfo->setJustificationType(Justification(juce::Justification::horizontallyCentred));
  fileInfo->setFont(juce::Font::Font(14.0));
  fileInfo->setColour(juce::Label::textColourId, Colours::darkgrey);

  // create the playback thread with this object as the source.
  pbthread=new MidiPlaybackThread(this, 100, 60.0, NULL, transport);
  // start the thread running, this will put its run loop in pause mode.
  pbthread->startThread();

  // create label for midi output menu
  addAndMakeVisible(midiPortLabel = new juce::Label(juce::String::empty, T("Playback device:")));

  // create midi output menu. done after the thread exists because it
  // sets the thread's output port
  addAndMakeVisible(midiPortMenu = new juce::ComboBox(String::empty));
  midiPortMenu->setEditableText(false);
  midiPortMenu->setTextWhenNoChoicesAvailable(T("No Midi output ports"));
  midiPortMenu->setTextWhenNothingSelected(T("Select Midi output port"));
  midiPortMenu->addListener(this);
  juce::StringArray devnames = MidiOutput::getDevices();
  for (int i=0; i<devnames.size(); i++)
  {
    midiPortMenu->addItem(devnames[i], i+1);        
    if (MidiOutPort::getInstance()->isOpen(i)) // select the same one as in the Audio menu
      midiPortMenu->setSelectedId(i+1);   // creates device and sets it in midi thread       
  }
  setVisible(true);
  setSize (400, 8+24+8+24+8+Transport::TransportHeight+8);
}

MidiFilePlayer::~MidiFilePlayer()
{
  pbthread->stopThread(100);
  delete pbthread;
  sequence.clear();
  deleteAndZero(fileInfo);
  deleteAndZero(transport);
  deleteAndZero(midiPortLabel);
  deleteAndZero(midiPortMenu);
  if (midiPort) deleteAndZero(midiPort);
  std::cout << "deleted MidiFilePlayer\n";
}
  
void MidiFilePlayer::setFile(File file)
{
  // pushes the stop and rewind buttons to halt playback and clears
  // any pending messages
  transport->setPausing();
  transport->setPlaybackPosition(0);

  fileDuration=0;
  fileLength=0;
  sequence.clear();
  midiFile=file;
  // create input stream for midi file and read it
  juce::FileInputStream* input=midiFile.createInputStream();
  if (input==0)
    return;
  juce::MidiFile midifile;
  if (!midifile.readFrom(*input))
  {
    //std::cout << "midi failed to read file\n";
    delete input;
    return;
  }
  int numtracks=midifile.getNumTracks();
  int timeformat=midifile.getTimeFormat();
  double smptetps=0.0; // smpte format ticks per second
  // juce time format is postive for beats and negative for SMTPE
  if (timeformat>0)
  {
    midifile.convertTimestampTicksToSeconds();    
  }
  else
  {
    // determine the smpte ticks per second. juce smpte frames per
    // second is negative upper byte
    int fps=0xFF-((timeformat & 0xFF00) >> 8)+1;
    int tpf=timeformat & 0xFF;
    smptetps=fps*tpf;
  }
  // iterate all the tracks in the file and merge them into our
  // empty playback sequence.
  for (int track=0; track <numtracks; track++)
  {
    const juce::MidiMessageSequence* seq=midifile.getTrack(track);
    // if file is in SMPTE FRAMES then convert it to seconds by
    // hand. (juce's convertTimestampTicksToSeconds doesnt work for
    // SMPTE format.)
    if (timeformat<0) // is smpte
    {
      for (int i=0; i<seq->getNumEvents(); i++)
      {
        juce::MidiMessageSequence::MidiEventHolder* h=seq->getEventPointer(i);
        double t=h->message.getTimeStamp();
        h->message.setTimeStamp(t/smptetps);
      }
    }
    // merge file track into our sigle playback sequence
    sequence.addSequence(*seq, 0.0, 0.0, seq->getEndTime()+1);
    sequence.updateMatchedPairs();
  }

  // the file may include more than the note ons and offs so the
  // file length may not reflect the actual number of events that we
  // play. set file playback duration to the very last note event
  fileLength=sequence.getNumEvents();
  for (int i=sequence.getNumEvents()-1; i>=0; i--)
    if (sequence.getEventPointer(i)->message.isNoteOnOrOff())
    {
      fileDuration=sequence.getEventPointer(i)->message.getTimeStamp();
      break;
    }
  // set the playback range to our upper bounds, note that the
  // length may include meta message
  pbthread->setPlaybackLimit(fileDuration, sequence.getNumEvents());
  pbthread->setMidiOutputPort(MidiOutPort::getInstance()->device);
  juce::String info (midiFile.getFileName());
  info << ": tracks=" << numtracks  << ", dur=" << fileDuration;
  fileInfo->setText(info, false);
}

const juce::File& MidiFilePlayer::getFile()
{
  return midiFile;
}

///////////////////
// Gui Callbacks //
///////////////////

void MidiFilePlayer::resized()
{
  int margin=8, lineheight=24;
  int x=margin, y=margin;
  int width=getWidth();
  int height=getHeight();
  int labwid = midiPortLabel->getFont().getStringWidthFloat(midiPortLabel->getText())+10;

  fileInfo->setBounds(margin, margin, getWidth()-16, lineheight);
  y += lineheight+margin;
  midiPortLabel->setBounds(x, y, labwid, lineheight);
  midiPortMenu->setBounds(midiPortLabel->getRight()+8, y, 200, lineheight);
  y += margin+lineheight;
  transport->setBounds(margin,y,getWidth()-(margin*2), Transport::TransportHeight);
}

void MidiFilePlayer::comboBoxChanged (ComboBox* combobox)
{
  int index=combobox->getSelectedItemIndex();
  pbthread->setMidiOutputPort(0);
  deleteAndZero(midiPort);
  midiPort=MidiOutput::openDevice(index);
  if (midiPort)
    pbthread->setMidiOutputPort(midiPort);      
}

//////////////////////////
// Transport Callbacks //
/////////////////////////

void MidiFilePlayer::play(double position)
{
  std::cout << "MidiFilePlayer::play(pos=" << position << ")\n" ;
  pbthread->setPaused(false);
}

// this function is called by the transport to stop playing

void MidiFilePlayer::pause()
{
  std::cout << "MidiFilePlayer::pause()\n" ;
  pbthread->setPaused(true);
}

void MidiFilePlayer::positionChanged(double position, bool isPlaying, int dir)
{
  std::cout << "MidiFilePlayer::positionChanged(" << position << ", " << isPlaying << ")\n" ;
  // convert the normalized position into a real position in the
  // playback sequence and update the MidiPlaybackThread's playback
  // position
  double newtime=position * fileDuration;
  int newindex = sequence.getNextIndexAtTime(newtime);
  // 
  if (isPlaying)
  {
    pbthread->pause();
  }
  pbthread->clear();
  pbthread->setPlaybackPosition(newtime,newindex);
  if (isPlaying)
  {
    pbthread->play();
  }
}

// this function is called by the transport when the tempo slider is moved

void MidiFilePlayer::tempoChanged(double tempo, bool isPlaying)
{
  std::cout << "MidiFilePlayer::tempoChanged(" << tempo << ", " << isPlaying << ")\n" ;
  pbthread->setTempo(tempo);
}

/////////////////////////////////
// MidiPlaybackThread Callback //
/////////////////////////////////

void MidiFilePlayer::addMidiPlaybackMessages(MidiPlaybackThread::MidiMessageQueue& queue, 
                                             MidiPlaybackThread::PlaybackPosition& position)
{
  // add noteOns for the current playback time and index then update
  // the index for the next runtime

  //std::cout << "addMidiPlaybackMessages beat=" << position.time << " index=" << position.index << "\n";
  int index=index=position.index;
  for (; index<position.length; index++)
  {
    MidiMessageSequence::MidiEventHolder* ev=sequence.getEventPointer(index);
    // skip over non-channel messages
    if (ev->message.getChannel()<1)
      continue;
    // skip over noteOffs because we add by pairs with noteOns
    if (ev->message.isNoteOff())
      continue;
    if (ev->message.getTimeStamp() <= position.beat)
    {
      queue.addMessage(new MidiMessage(ev->message));
      //MidiPlaybackThread::printMessage(ev->message);
      if (ev->noteOffObject)
      {
        queue.addMessage(new MidiMessage(ev->noteOffObject->message));            
        //MidiPlaybackThread::printMessage(ev->noteOffObject->message);
      }
    }
    else
      break;
  }
  // index is now the index of the next (future) event or length
  position.index=index;
}




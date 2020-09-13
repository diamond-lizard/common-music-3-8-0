/*=======================================================================*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the Lisp Lesser Gnu Public License. The text
  of this agreement is available at http://www.cliki.net/LLGPL
 *=======================================================================*/

#ifndef Transport_h
#define Transport_h

#include "Libraries.h"

/** An audio control component. When the user clicks on a Transport's
    buttons or moves its sliders the Transport::Listener's methods are
    triggered to effect playback changes.  A Transport provides 5
    buttons: Rewind, Back, Play/Pause, Forward, GoToEnd, a slider for
    scrolling through the playback sequence and an optional tempo
    slider with associated bpm label.  A Transport can be triggered
    from the main message thread by calling its underlying
    setPlaying(), setPausing(), setPlaybackPosition() and setTempo()
    methods.  To trigger transport outside of the main thread use its
    asynchronous sendMessage() method. **/

class Transport : public juce::Component, 
  public juce::AsyncUpdater,
  public juce::ButtonListener,
  public juce::SliderListener
{
 public:

  /** Pass one of these into Transport to configure the tempo slider. **/

  struct TempoConfig
  {
    TempoConfig(double initial=60.0, double minimum=40.0, double maximum=208.0, 
                double increment=1.0, double midpoint=92.0, String suffix=juce::String(" BPM"),
                int decimals=0)
      : tempoInitial (initial),
        tempoMinimum (minimum), 
        tempoMaximum (maximum),
        tempoIncrement (increment),
        tempoMidPoint (midpoint),
        tempoSuffix(suffix),
        tempoDecimals(decimals)
    {
    }
    ~TempoConfig()
    {
    }
    double tempoInitial;
    double tempoMinimum;
    double tempoMaximum;
    double tempoIncrement;
    double tempoMidPoint;
    String tempoSuffix;
    int tempoDecimals;
  };
    
  /** Transport button ids. **/

  enum TransportButtonIds
  {
    RewindButton=1, BackButton, PlayPauseButton, ForwardButton, GoToEndButton, SliderButton
  };

  /** Various aspects of the transport display. **/

  enum TransportGeometry
  {
    ButtonWidth = 44,  /// The width of the transport buttons
    ButtonHeight = 24, /// The height of the transport buttons
    Margin = 4,        /// The inset space around edges
    NumButtons = 5,    /// The number of buttons on transport
    TransportWidthNoTempo = Margin + (ButtonWidth*NumButtons) + Margin,
    TransportWidthWithTempo = TransportWidthNoTempo + (ButtonHeight*5),
    TransportHeight = Margin + (ButtonHeight*2) + Margin,

  };

 /** Transport message ids. Use these with sendMessage to control the
      transport from other threads.  **/

  enum TransportMessageIds
  {
    SetPlaying,
    SetPausing,
    SetPlaybackPosition,
    SetPlaybackTempo
  };

  /** A class for receiving callbacks from a Transport. **/

  class Listener
  {
  public:

    virtual ~Listener() {}

    /** Called when the user presses the transport's Play button. The
        playback position is a normalized value 0.0 to 1.0. **/

    virtual void play(double position) = 0;

    /** Called when the user presses the transport's Pause button. **/

    virtual void pause(void) = 0;

    /** Called when the user moves the transport's slider button. The
        playback position is a normalized value 0.0 to 1.0 and
        isPlaying is true if the transport is currently marked as
        playing. Direction indicates which direction the transport
        moved if one of directional buttons is pressed: 1 is forward,
        -1 is backward and 0 means the tempo slider was used to
        trigger the callback. **/

    virtual void positionChanged(double position, bool isPlaying, int dir) = 0;

    /** Called when the user moves the tempo slider. Tempo is the new
        tempo that was just set and and isPlaying is true if the
        transport is currently marked as playing. **/

    virtual void tempoChanged(double tempo, bool isPlaying) = 0;

  };
  
  /** Transport constructor. To create a Transport pass it a
      Transport::Listener with its play(), pause() , positionChanged()
      and tempoChanged() callbacks implemented. If a non-zero tempo
      value is specfied then the transport will contain a tempo
      slider to alter the playback rate while audio is running. **/

  Transport (Listener* transportListener, TempoConfig* tempo=NULL, bool positionSlider=true)
    : playing(false),
      listener(transportListener),
      buttonColor (juce::Colour(90,90,120)),
      toggleColor (juce::Colour(60,60,180)),
      buttonPlayPause(0),
      buttonRewind(0),
      buttonBack(0),
      buttonForward(0),
      buttonGoToEnd(0),
      sliderPosition(0),
      sliderTempo(0),
      labelTempo(0),
      tempoSuffix(juce::String::empty),
      tempoPrecision(1.0)
    {
      int connectLeft = juce::Button::ConnectedOnLeft;
      int connectBoth = juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight;
      int connectRight = juce::Button::ConnectedOnRight;     
      juce::DrawableButton::ButtonStyle buttonStyle = juce::DrawableButton::ImageOnButtonBackground;
      
      addAndMakeVisible(buttonRewind = new juce::DrawableButton(juce::String("Rewind"), buttonStyle));
      buttonRewind->setBackgroundColours(buttonColor, toggleColor);
      buttonRewind->setConnectedEdges(connectRight);
      drawRewind(buttonRewind);
      buttonRewind->addListener(this);
	
      addAndMakeVisible(buttonBack = new juce::DrawableButton(juce::String("Back"), buttonStyle));
      buttonBack->setBackgroundColours(buttonColor, toggleColor);
      buttonBack->setConnectedEdges(connectBoth);
      drawBack(buttonBack);
      buttonBack->addListener(this);
	
      addAndMakeVisible(buttonPlayPause = new juce::DrawableButton(juce::String("PlayPause"), buttonStyle));
      buttonPlayPause->setBackgroundColours(buttonColor, toggleColor);
      buttonPlayPause->setConnectedEdges(connectBoth);
      drawPlay(buttonPlayPause);
      buttonPlayPause->addListener(this);
	
      addAndMakeVisible(buttonForward = new juce::DrawableButton(juce::String("Forward"), buttonStyle));
      buttonForward->setBackgroundColours(buttonColor, toggleColor);
      buttonForward->setConnectedEdges(connectBoth);
      drawForward(buttonForward);
      buttonForward->addListener(this);

      addAndMakeVisible(buttonGoToEnd = new juce::DrawableButton(juce::String("GoToEnd"), buttonStyle));
      buttonGoToEnd->setBackgroundColours(buttonColor, toggleColor);
      buttonGoToEnd->setConnectedEdges(connectLeft);
      drawGoToEnd(buttonGoToEnd);
      buttonGoToEnd->addListener(this);
	
      if (positionSlider)
      {
        addAndMakeVisible(sliderPosition = new juce::Slider(juce::String("Position")));
        sliderPosition->setSliderStyle(juce::Slider::LinearHorizontal);
        sliderPosition->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sliderPosition->setColour(juce::Slider::thumbColourId,buttonColor);
        sliderPosition->setRange(0, 1.0);
        sliderPosition->addListener(this);
      }
 
      // basic width is 5 buttons and height is 2 lines with
      // margins all around
      int width = Margin + (ButtonWidth*5)  + Margin;
      int height = Margin + (ButtonHeight*2) + Margin;

      if (tempo)
      {
        /* Optional tempo display consists of a rotary slider and an
           associated number label. The slider claims a square of
           ButtonHeight*2 so that it fills the available vertical
           space and is positioned 1/2 lineheight to the right of the
           GoToEnd button. The bpm label abutts the tempo slider. Its
           width sized to fit the largest possible number display. */

        addAndMakeVisible(sliderTempo = new juce::Slider(juce::String("Tempo")));
        sliderTempo->setSliderStyle(juce::Slider::Rotary);
        // disable normal textbox, we'll add our own
        //        sliderTempo->setTextBoxStyle(Slider::TextBoxRight, true, 120, 24);
        //        sliderTempo->setTextValueSuffix(juce::String(" BPM"));
        sliderTempo->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sliderTempo->setColour(juce::Slider::thumbColourId, buttonColor);
        sliderTempo->setColour(juce::Slider::rotarySliderFillColourId, buttonColor);
        sliderTempo->setColour(juce::Slider::rotarySliderOutlineColourId, buttonColor);
        sliderTempo->setRange(tempo->tempoMinimum, tempo->tempoMaximum, tempo->tempoIncrement);
        sliderTempo->setValue(tempo->tempoInitial, true, true);
        if (tempo->tempoMidPoint > tempo->tempoMinimum && tempo->tempoMidPoint < tempo->tempoMaximum)
          sliderTempo->setSkewFactorFromMidPoint(tempo->tempoMidPoint);          

        // add custom tempo label
        addAndMakeVisible(labelTempo = new juce::Label(juce::String::empty, juce::String::empty));
        //        labelTempo->setVisible(false);
        tempoSuffix=tempo->tempoSuffix;
        labelTempo->setColour(juce::Label::textColourId, buttonColor);
        // determine the maximum width of the label display
        juce::String maxstr=String(juce::roundToInt(tempo->tempoMaximum));
        if (tempo->tempoDecimals>0)
        {
          maxstr << ".";
          for (int i=0;i<tempo->tempoDecimals;i++)
          {
            maxstr << "0";
            tempoPrecision = tempoPrecision * 10.0;
          }
        }
        maxstr << tempoSuffix;
        int wid=labelTempo->getFont().getStringWidthFloat(maxstr)+10;
        labelTempo->setSize(wid, ButtonHeight*2);     //(ButtonHeight*2)+halfheight,  ButtonHeight*2);      
        printTempo(tempo->tempoInitial);
        sliderTempo->addListener(this);
        width += wid; //(ButtonHeight*5);
      }
      setVisible(true);
      setSize(width, height);
    }
  
  /** Transport destructor. **/

  virtual ~Transport()
  {
    deleteAllChildren();
  }

 /** Returns true if the transport is playing otherwise false. This
     method is not thread safe. **/

  bool isPlaying()
  {
    return playing;
  }

  /** Marks transport as playing, flips to the "pause" icon for the
      user and calls the play() method if triggerAction is true and
      the transport is not already playing. This method is not thread
      safe, use sendMessage() for that. **/

  void setPlaying(bool triggerAction=true)
  {
    //std::cout << "in setPlaying, trigger action is " << triggerAction << " playing status was " << playing << "\n";
    bool toggled=(playing==false);
    playing=true;
    drawPause(buttonPlayPause);
    if (triggerAction && toggled)
    {
      //      std::cout << "triggering play...\n";
      listener->play(getPlaybackPosition());
    }
  }
  
  /** Marks transport as pausing, displays the "play" icon for the
      user and calls the pause() method if triggerAction is true and
      the transport is currently playing. This method is not thread
      safe, use sendMessage() for that. **/

  void setPausing(bool triggerAction=true)
  { 
    //std::cout << "in setPausing, trigger action is " << triggerAction << " playing status was " << playing << "\n";
    bool toggled=(playing==true);
    playing=false;
    drawPlay(buttonPlayPause);
    if (triggerAction && toggled)
    {
      //      std::cout << "triggering pause...\n";
      listener->pause();
    }
  }

  /** Sets the playback position as a normalized value 0.0 to 1.0. If
      triggerAction is true then the positionChanged() callback will
      be triggered. This method is not thread safe, use sendMessage()
      for that. **/

  void setPlaybackPosition(double position, bool triggerAction=true)
  {
    if (sliderPosition)
      sliderPosition->setValue(position, triggerAction, false);
  }

  /** Increments the playback position by the specified delta. If
      triggerAction is true then the positionChanged() callback will
      be triggred. This method is not thread safe, use sendMessage()
      for that. **/

  void incrementPlaybackPosition(double delta, bool triggerAction=true)
  {
    if (sliderPosition)
      setPlaybackPosition(sliderPosition->getValue()+delta, triggerAction);
  }

  /** Returns the position of the playback position slider on a
      normalized scale 0.0 to 1.0. This function is not thread
      safe. **/

  double getPlaybackPosition(void)
  {
    if (sliderPosition)
      return sliderPosition->getValue();
    return 0.0;
  }

  /** Sets the current playback tempo in BPM. If triggerAction is true
      then the tempoChanged() callback will be triggered. This function
      is not thread safe. **/

  void setPlaybackTempo(double tempo, bool triggerAction=true)
  {
    // tempo components are optional
    if (sliderTempo)
    {
      sliderTempo->setValue(tempo, triggerAction, false);
      // if we are not triggering the update so print the label by hand
      if (!triggerAction)
      {
        printTempo(tempo);
      }
    }
  }

  /** Returns the current playback tempo. This function is not thread safe. **/

  double getPlaybackTempo(void)
  {
    if (sliderTempo)
      return sliderTempo->getValue();
    return 0.0;
  }

  /** Implements thread-safe transport control. id
      is a message id, one of the button enums defined at the top of
      this file. The rest af the args are any data you need to
      pass. **/ 

  void sendMessage(int id, double d=0.0, int i=0, bool b=false)
  {
    juce::ScopedLock mylock(messages.getLock());
    messages.add(new TransportMessage(id, d, i, b));
    triggerAsyncUpdate();
  }

 private:

  bool playing;
  Listener* listener; 
  juce::Colour buttonColor;
  juce::Colour toggleColor;
  juce::DrawableButton* buttonPlayPause;
  juce::DrawableButton* buttonRewind;
  juce::DrawableButton* buttonBack;
  juce::DrawableButton* buttonForward;
  juce::DrawableButton* buttonGoToEnd;
  juce::Slider*         sliderPosition;
  juce::Slider*         sliderTempo;
  juce::Label*          labelTempo;
  juce::String tempoSuffix;
  double tempoPrecision;

  /** Internal support for messaging from other threads **/

  class TransportMessage
  {
  public:
    int type;
    double double1;
    int int1;
    bool bool1;
    TransportMessage (int typ, double d1, int i1, bool b1) : type(typ), double1 (d1), int1 (i1), bool1 (b1) {} 
    ~TransportMessage() {}
  };

  juce::OwnedArray<TransportMessage, juce::CriticalSection> messages;

  void handleAsyncUpdate ()
  {
    juce::ScopedLock mylock(messages.getLock());
    int size=messages.size();
    for (int i=0; i<size; i++)
    {
      TransportMessage* msg=messages.getUnchecked(i);
      switch (msg->type)
      {
      case SetPausing:
        //std::cout << "TransportMessage: SetPausing\n";
        setPausing(msg->bool1); // false means dont trigger action
        break;
      case SetPlaying:
        //std::cout << "TransportMessage: SetPlaying\n";
        setPlaying(msg->bool1); // false means dont trigger action
        break;
      case SetPlaybackPosition:
        //std::cout << "TransportMessage: SetPosition pos=" << msg->double1 << "\n";
        setPlaybackPosition(msg->double1, msg->bool1);
        break;
      case SetPlaybackTempo:
        //std::cout << "TransportMessage: SetPosition pos=" << msg->double1 << "\n";
        setPlaybackTempo(msg->double1, msg->bool1);
        break;
      default:
        break;
      }
    }
    if (size>0)
      messages.clear();
  }

  /** Centers the transport controls within the horizontal and
      vertical area of the Transport. **/

  void resized()
  {
    int viewWidth=getWidth();
    int viewHeight=getHeight();
    int buttonLeft=(viewWidth/2)-((int)(ButtonWidth*2.5));  // total of 5 buttons
    // a tempo slider adds an extra 5 lineHeight's of width
    if (sliderTempo)
      buttonLeft -= (int)(ButtonHeight*2.5);

    int buttonTop=(viewHeight/2)-ButtonHeight;
    // if there is no position slider shift the buttons down to middle of tempo component
    int fudge = (!sliderPosition) ? (ButtonHeight/2) : 0;
    buttonRewind->setBounds(buttonLeft, buttonTop+fudge, ButtonWidth, ButtonHeight);
    buttonBack->setBounds(buttonLeft + ButtonWidth, buttonTop+fudge, ButtonWidth, ButtonHeight);      
    buttonPlayPause->setBounds(buttonLeft + (ButtonWidth*2), buttonTop+fudge, ButtonWidth, ButtonHeight);
    buttonForward->setBounds(buttonLeft + (ButtonWidth*3), buttonTop+fudge, ButtonWidth, ButtonHeight);
    buttonGoToEnd->setBounds(buttonLeft + (ButtonWidth*4), buttonTop+fudge, ButtonWidth, ButtonHeight);    
    if (sliderPosition)
    {
      sliderPosition->setBounds(buttonLeft, buttonTop + ButtonHeight, (ButtonWidth*5), ButtonHeight);
    }
    if (sliderTempo)
    {
      // the optional tempo components are squares of 2 lineheights
      // per side positioned one lineheight to the right of the
      // rightmost transport button.
      int halfheight=ButtonHeight/2;
      sliderTempo->setBounds(buttonGoToEnd->getRight()+halfheight, buttonTop , ButtonHeight*2,  ButtonHeight*2);
      //      labelTempo->setBounds(sliderTempo->getRight(), sliderTempo->getY(), (ButtonHeight*2)+halfheight,  ButtonHeight*2);
      labelTempo->setTopLeftPosition(sliderTempo->getRight(), sliderTempo->getY());
    }
  }

  /** Internal juce callback implements button clicked actions. **/

  virtual void buttonClicked(juce::Button* button)
  {
    static bool isShowingPlayNotPause = true;
    
    if(button == buttonPlayPause)
    {
      //Toggle the play/pause button.
      isShowingPlayNotPause = !isShowingPlayNotPause;
      
      if(isPlaying())
      {
        setPausing();
      }
      else
      {
        setPlaying();
      }
    }
    else if(button == buttonRewind)
    {
      if (sliderPosition)
        sliderPosition->setValue(0);
      else
        listener->positionChanged(0, isPlaying(), 0);
    }
    else if(button == buttonBack)
    {
      if (sliderPosition)
        sliderPosition->setValue(sliderPosition->getValue() - 0.10);
      else
        listener->positionChanged(-0.1, isPlaying(), -1);
    }
    else if(button == buttonForward)
    {
      if (sliderPosition) 
        sliderPosition->setValue(sliderPosition->getValue() + 0.10);
      else
        listener->positionChanged(0.1, isPlaying(), 1);
    }
    else if(button == buttonGoToEnd)
    {
      if (sliderPosition)
        sliderPosition->setValue(1);
      else
        listener->positionChanged(1, isPlaying(), 0);
    }
  }
  
  /** Internal juce callback implements slider moved actions. **/

  virtual void sliderValueChanged(juce::Slider *slider)
  {
    if (slider == sliderPosition)
      listener->positionChanged(sliderPosition->getValue(), isPlaying(), 0);
    else if (slider == sliderTempo)
    {
      double val=sliderTempo->getValue();
      //      labelTempo->setText(juce::String(juce::roundToInt(val))+juce::String(" BPM"), false);
      printTempo(val);
      listener->tempoChanged(val, isPlaying());
    }
  }

  void printTempo(double tempo)
  {
    if (tempoPrecision==1.0)
      labelTempo->setText(juce::String(juce::roundToInt(tempo)) + tempoSuffix, false);
    else
    {
      double n = tempoPrecision; //1000.0; //(double)(10 ^ tempoDecimals);
      double d=((double)juce::roundToInt(tempo*n))/n;
      std::cout << "precision=" << n << ", value=" <<d << "\n";
      juce::String str=juce::String(d);
      labelTempo->setText(str + tempoSuffix, false);
    }

  }

  /** Internal button drawing. **/

  void drawRewind(juce::DrawableButton* b)
  {
    juce::DrawablePath imageRewind;
    juce::Path pathRewind;
    pathRewind.addRectangle(-1.8f,0,0.3f,2.0f);
    pathRewind.addTriangle(0,0, 0,2.0f, -1.2f,1.0f);
    imageRewind.setPath(pathRewind);
    juce::FillType ft (juce::Colours::white);
    imageRewind.setFill(ft);
    b->setImages(&imageRewind);
  }

  /** Internal button drawing. **/

  void drawBack(juce::DrawableButton* b)
  {
    juce::DrawablePath imageBack;
    juce::Path pathBack;
    pathBack.addTriangle(0,0,0,2.0f,-1.2f,1.0f);
    pathBack.addTriangle(1.2f,0,1.2f,2.0f,0,1.0f);
    imageBack.setPath(pathBack);
    juce::FillType ft (juce::Colours::white);
    imageBack.setFill(ft);
    b->setImages(&imageBack);
  }
  
  /** Internal button drawing. **/

  void drawPlay(juce::DrawableButton* b)
  {
    juce::DrawablePath imagePlay;
    juce::Path pathPlay;
    pathPlay.addTriangle(0,0,0,2.0f,1.2f,1.0f);
    imagePlay.setPath(pathPlay);
    juce::FillType ft (juce::Colours::white);
    imagePlay.setFill(ft);
    b->setImages(&imagePlay);
  }
  
  /** Internal button drawing. **/

  void drawPause(juce::DrawableButton* b)
  {
    juce::DrawablePath imagePause;
    juce::Path pathPause;
    pathPause.addRectangle(0,0,0.3f,1.0f);
    pathPause.addRectangle(0.6f,0,0.3f,1.0f);
    imagePause.setPath(pathPause);
    juce::FillType ft (juce::Colours::white);
    imagePause.setFill(ft);
    b->setImages(&imagePause);
  }
  
  /** Internal button drawing. **/

  void drawForward(juce::DrawableButton* b)
  {
    juce::DrawablePath imageForward;
    juce::Path pathForward;
    pathForward.addTriangle(0,0,0,2.0f,1.2f,1.0f);
    pathForward.addTriangle(-1.2f,0,-1.2f,2.0f,0,1.0f);
    imageForward.setPath(pathForward);
    juce::FillType ft (juce::Colours::white);
    imageForward.setFill(ft);
    b->setImages(&imageForward);
  }
    
  /** Internal button drawing. **/

  void drawGoToEnd(juce::DrawableButton* b)
  {
    juce::DrawablePath imageGoToEnd;
    juce::Path pathGoToEnd;
    pathGoToEnd.addTriangle(-1.4f, 0, -1.4f,2.0f, -0.2f,1.0f);
    pathGoToEnd.addRectangle(0.1f, 0, 0.3f, 2.0f);
    imageGoToEnd.setPath(pathGoToEnd);
    juce::FillType ft (juce::Colours::white);
    imageGoToEnd.setFill(ft);
    b->setImages(&imageGoToEnd);
  }

};

#endif

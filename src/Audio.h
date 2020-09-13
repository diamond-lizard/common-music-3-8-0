/*=======================================================================*
Copyright (C) 2008-2010 Rick Taube.                                        
This program is free software; you can redistribute it and/or modify  
it under the terms of the Lisp Lesser Gnu Public License. The text of 
this agreement is available at http://www.cliki.net/LLGPL             
*=======================================================================*/
#ifndef AUDIO_H
#define AUDIO_H

#include "Libraries.h"

class FilterGraph;

class AudioManager : public AudioDeviceManager
{
  OwnedArray<FilterGraph,CriticalSection> pluginGraphs;

public:

  AudioManager();
  ~AudioManager();

  /////////////////////////
  // Audio Plugin Graphs //
  /////////////////////////

  /** Creates a new empty plugin graph and adds it to the end of the graph array. **/
  void newPluginGraph();
  /** Loads a plugin graph. If no file is provided an Open... dialog is opened. **/
  bool loadPluginGraph(File file=File::nonexistent);
  /** Deletes the specified graph. **/
  void deletePluginGraph(int graphIndex);
  //  /** Delete all plugin graphs. **/
  //  void deletePluginGraphs();
  /** Returns true if at least one plugin graph exists. **/
  bool isPluginGraph();
  /** Returns the number of existing graphs. **/
  int numPluginGraphs();
  /** Returns true if graph at index can make sound. **/
  bool isPluginGraphEnabled(int index);
  /** Enables or disables audio playback in the specified graph. **/
  void setPluginGraphEnabled(int index, bool enabled);
  /** Returns the name of the specified graph. **/
  juce::String getPluginGraphName(int index);
  /** Prints the nodes and connections of the specified graph in the Console window. **/
  void printPluginGraph(int index);
  /** Sends a midi message to the current plugin graph. **/
  bool sendMessageToPluginGraph(const MidiMessage &message);

  /** Returns the number of plugin node in the specified graph. **/
  int numPlugins(int graphIndex);
  /** Returns the name of the plugin in the specified node and graph. **/
  juce::String getPluginName(int graphIndex, int nodeIndex);
  /** Returns true if the plugin in the specified node and graph provides an editor. **/
  bool hasPluginEditor(int graphIndex, int nodeIndex);
  /** Opens a plugin editor window for the specified node and graph. **/
  void openPluginEditor(int graphIndex, int nodeIndex);

  void openAudioSettings();
  void stopAudioPlayback();
  void openAudioFilePlayer(File fileToOpen=juce::File::nonexistent, bool play=false);

  juce_DeclareSingleton (AudioManager, true)

};

#endif

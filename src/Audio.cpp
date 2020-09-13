/*=======================================================================*
  Copyright (C) 2008-2010 Rick Taube.                                        
  This program is free software; you can redistribute it and/or modify  
  it under the terms of the Lisp Lesser Gnu Public License. The text of 
  this agreement is available at http://www.cliki.net/LLGPL             
 *=======================================================================*/

#include "Enumerations.h"
#include "Preferences.h"
#include "Audio.h"
#include "Console.h"
#include "FilterGraph.h"
#include "AudioFilePlayer.h"

/*=======================================================================*
                            Global Audio Manager
 *=======================================================================*/
 
juce_ImplementSingleton(AudioManager)

AudioManager::AudioManager()
{
  AudioPluginFormatManager::getInstance()->addDefaultFormats();
  AudioPluginFormatManager::getInstance()->addFormat (new InternalPluginFormat());
}

AudioManager::~AudioManager()
{
  //deletePluginGraphs();
  while (numPluginGraphs()>0)
    deletePluginGraph(0);
  closeAudioDevice();
}

/*=======================================================================*
                            Audio File Player Window
 *=======================================================================*/

void AudioManager::stopAudioPlayback()
{
  // TODO: cycle through audiofileplayer windows and stop them playing
}

void AudioManager::openAudioSettings()
{
  // Open an AudioDeviceSelectorComponent but without midi selections
  // (these are handled by the Midi Manger)
  AudioDeviceSelectorComponent comp (*this, 0, 256, 0, 256, false, false, true, false);

  comp.setSize(500, 450);
  /*  int b=0, r=0;
  for (int i=0; i<comp.getNumChildComponents(); i++)
  {
    Component* c=comp.getChildComponent(i);
    if (b < c->getBottom())
      b=c->getBottom();
    if (r < c->getRight())
      r=c->getRight();
  }
  comp.setSize(r+10, b+20);
  */
  DialogWindow::showModalDialog ("Audio Settings", &comp, NULL, Colour(0xffe5e5e5), true);
  AudioDeviceSetup setup;
  getAudioDeviceSetup(setup);
  //std::cout << "setting preferences to " <<  setup.bufferSize << "\n";
  //  Preferences::getInstance()->setIntProp(T("AudioPlaybackBufferSize"), setup.bufferSize);
}

//==============================================================================
//                             Plugin Graphs
//==============================================================================

void AudioManager::newPluginGraph()
{
  FilterGraph* graph=new FilterGraph();
  pluginGraphs.add(graph);
  // graph has to be enabled before any plugins are loaded or else the
  // audio connections will not be made!
  graph->player.setProcessor(&graph->getGraph());
  addAudioCallback(&graph->player);
  addMidiInputCallback(String::empty, &(graph->player.getMidiMessageCollector()));
}

bool AudioManager::loadPluginGraph(File file)
{
  bool loaded=false;
  newPluginGraph(); // add a new graph at end of array
  FilterGraph* graph=pluginGraphs.getLast();
  if (file==File::nonexistent)
    loaded=graph->loadFromUserSpecifiedFile(true);
  else
    loaded=graph->loadFrom(file, true);
  if (!loaded)
  {
    deletePluginGraph(numPluginGraphs()-1);
    return false;
  }
  // only allow one plugin graph for now so if we had a plugin the new
  // one is at the end of the array and the old one is still at [0].
  if (numPluginGraphs()>1)
  {
    deletePluginGraph(0);
  }
  // after delete the new graph is at [0]
  printPluginGraph(0);
  Preferences::getInstance()->recentlyLoadedGraphs.addFile(graph->getFile());
  return true;
}

void AudioManager::deletePluginGraph(int index)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(index);
  removeAudioCallback(&graph->player);
  removeMidiInputCallback(String::empty, &graph->player.getMidiMessageCollector());
  graph->player.setProcessor(NULL);
  pluginGraphs.remove(index, true);
}

int AudioManager::numPluginGraphs()
{
  return pluginGraphs.size();
}

bool AudioManager::isPluginGraph()
{
  return pluginGraphs.size()>0;
}

bool AudioManager::isPluginGraphEnabled(int index)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(index);
  return (graph->player.getCurrentProcessor() != NULL);
}

void AudioManager::setPluginGraphEnabled(int index, bool isEnabled)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(index);
  if (isEnabled)
  {
    if (graph->player.getCurrentProcessor() == NULL)
    {
      //      std::cout << "enabling graph " << index << "\n";
      graph->player.setProcessor(&graph->getGraph());
      ////      addAudioCallback(&graph->player);
      ////      addMidiInputCallback(String::empty, &(graph->player.getMidiMessageCollector()));
      //      graph->enabled=true;
    }
  }
  else
  {
    if (graph->player.getCurrentProcessor() != NULL)
    {
      //      std::cout << "disabling graph " << index << "\n";
      ////      removeAudioCallback(&graph->player);
      ////      removeMidiInputCallback(String::empty, &graph->player.getMidiMessageCollector());
      graph->player.setProcessor(NULL);
      //      graph->enabled=false;
    }
  }
}

bool AudioManager::sendMessageToPluginGraph(const MidiMessage &message)
{
  // using [] so that if no graphs are loaded a NULL results instead
  // of a segfault! (only one graph supported so index is always 0)
  if (FilterGraph* graph=pluginGraphs[0])
  {
    graph->player.getMidiMessageCollector().addMessageToQueue(message);
    return true;
  }
  return false;
}

juce::String AudioManager::getPluginGraphName(int index)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(index); 
  return graph->getDocumentTitle();
}

void AudioManager::printPluginGraph(int index)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(index);
  StringArray nodeNames;
  Array<uint32> nodeIds;
  juce::String msg;

  msg << "\nPlugin Graph '" << graph->getDocumentTitle() << "' (" 
      << graph->getNumFilters() << " nodes, " 
      << graph->getNumConnections() << " connections)\n";
  msg << "Nodes: \n";
  for (int i=0; i<graph->getNumFilters(); i++)
  {
    if (AudioPluginInstance* ap = dynamic_cast <AudioPluginInstance*> (graph->getNode(i)->getProcessor()))
    {
      PluginDescription pd;
      ap->fillInPluginDescription(pd);
      nodeNames.add(pd.name);
      nodeIds.add(graph->getNode(i)->nodeId); // this is nodeId in newer juces...
      msg << "  " << pd.name << "\n";
    }
  }
  msg << "Connections:\n";
  for (int i=0; i<graph->getNumConnections(); i++)
  {
    const AudioProcessorGraph::Connection* conn=graph->getConnection(i);
    String from=nodeNames[nodeIds.indexOf(conn->sourceNodeId)];
    String dest=nodeNames[nodeIds.indexOf(conn->destNodeId)];
    if (conn->sourceChannelIndex != AudioProcessorGraph::midiChannelIndex)
    {
      from << "[" << conn->sourceChannelIndex << "]";
      dest << "[" << conn->destChannelIndex << "]";
    }
    msg << "  " << from << "\t->\t" << dest << "\n";
  }
  Console::getInstance()->printOutput(msg);
}

//
/// Plugin (graph node) Methods
//

int AudioManager::numPlugins(int index)
{
  return pluginGraphs.getUnchecked(index)->getNumFilters();
}

juce::String AudioManager::getPluginName(int graphIndex, int nodeIndex)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(graphIndex);
  if (AudioPluginInstance* plugin = dynamic_cast <AudioPluginInstance*> (graph->getNode(nodeIndex)->getProcessor()))
  {
    PluginDescription pd;
    plugin->fillInPluginDescription(pd);
    return pd.name;
  }
  return juce::String::empty;
}

bool AudioManager::hasPluginEditor(int graphIndex, int nodeIndex)
{
  FilterGraph* graph=pluginGraphs.getUnchecked(graphIndex);
  if (AudioPluginInstance* plugin = dynamic_cast <AudioPluginInstance*> (graph->getNode(nodeIndex)->getProcessor()))
    return plugin->hasEditor();
  return false;
}  

class PluginEditorWindow : public DocumentWindow
{
public:
  PluginEditorWindow(juce::String title, juce::AudioProcessorEditor* editor)
    : juce::DocumentWindow(title, juce::Colour(0xffe5e5e5), 
                           juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton)
  {
    setUsingNativeTitleBar(true);
    setDropShadowEnabled(true);
    setContentOwned(editor,true);
    setCentreRelative(0.5,0.5);
    getContentComponent()->setVisible(true);
    setVisible(true);  
    WindowTypes::setWindowType(this,WindowTypes::PluginEditor);
  }
  
  ~PluginEditorWindow()
  {
  }

  void closeButtonPressed()
  {
    delete this;
  }
};

void AudioManager::openPluginEditor(int graphIndex, int nodeIndex)
{
  juce::String name=getPluginName(graphIndex, nodeIndex);
  // see if an editor is already open for this plugin
  for (int i=0; i<juce::TopLevelWindow::getNumTopLevelWindows(); i++)
  {
    juce::TopLevelWindow* w=juce::TopLevelWindow::getTopLevelWindow(i);
    if (WindowTypes::isWindowType(w, WindowTypes::PluginEditor) &&
        (w->getName() == name))
    {
      w->getTopLevelComponent()->toFront(true);
      return;
    }
  }
  AudioProcessorGraph::Node* node=pluginGraphs.getUnchecked(graphIndex)->getNode(nodeIndex);
  if (juce::AudioPluginInstance* plugin=dynamic_cast<juce::AudioPluginInstance*> (node->getProcessor()))
    if (juce::AudioProcessorEditor* editor=plugin->createEditorIfNeeded())
    {
      new PluginEditorWindow(name,editor);
      return;
    }
  Console::getInstance()->printWarning("No plugin editor for " + name + "\n");
}

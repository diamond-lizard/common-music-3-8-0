#ifndef FILTERGRAPH_H
#define FILTERGRAPH_H

#include "Libraries.h"

class FilterInGraph;
class FilterGraph;

//==============================================================================
// InternalPluginFormat
//==============================================================================

/** Manages the graph's internal IOProcessor nodes. */

class InternalPluginFormat   : public AudioPluginFormat
{

public:

  InternalPluginFormat()
  {
    {
      AudioProcessorGraph::AudioGraphIOProcessor p (AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
      p.fillInPluginDescription (audioOutDesc);
    }

    {
      AudioProcessorGraph::AudioGraphIOProcessor p (AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
      p.fillInPluginDescription (audioInDesc);
    }

    {
      AudioProcessorGraph::AudioGraphIOProcessor p (AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
      p.fillInPluginDescription (midiInDesc);
    }
  }

  ~InternalPluginFormat()
  {
  }

  enum InternalFilterType
    {
      audioInputFilter = 0,
      audioOutputFilter,
      midiInputFilter,
      endOfFilterTypes
    };

  const PluginDescription* getDescriptionFor (const InternalFilterType type)
  {
    switch (type)
    {
    case audioInputFilter:      return &audioInDesc;
    case audioOutputFilter:     return &audioOutDesc;
    case midiInputFilter:       return &midiInDesc;
    default:                    break;
    }
    return 0;
  }

  void getAllTypes (OwnedArray <PluginDescription>& results)
  {
    for (int i = 0; i < (int) endOfFilterTypes; ++i)
      results.add (new PluginDescription (*getDescriptionFor ((InternalFilterType) i)));
  }

  String getName() const
  {
    return "Internal";
  }

  bool fileMightContainThisPluginType (const String&)         
  {
    return false;
  }

  FileSearchPath getDefaultLocationsToSearch()                
  {
    return FileSearchPath();
  }

  void findAllTypesForFile (OwnedArray <PluginDescription>&, const String&)
  {
  }

  bool doesPluginStillExist (const PluginDescription&)
  {
    return true;
  }

  String getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
  {
    return fileOrIdentifier;
  }

  StringArray searchPathsForPlugins (const FileSearchPath&, bool)
  {
    return StringArray();
  }

  /** Create internal plugin representations of the processor graph's
      IO nodes. This allows user plugins to connect to audio/midi IO
      using graph node connections.  **/

  AudioPluginInstance* createInstanceFromDescription (const PluginDescription& desc)
  {
    if (desc.name == audioOutDesc.name)
    {
      return new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    }
    else if (desc.name == audioInDesc.name)
    {
      return new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    }
    else if (desc.name == midiInDesc.name)
    {
      return new AudioProcessorGraph::AudioGraphIOProcessor (AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
    }
    return 0;
  }

private:

  PluginDescription audioInDesc;
  PluginDescription audioOutDesc;
  PluginDescription midiInDesc;
};

//==============================================================================
// FilterConnection
//==============================================================================

/** Represents a connection between two pins in a FilterGraph. */

class FilterConnection
{

public:

  FilterConnection (FilterGraph& ownr)
    : owner (ownr)
  {
  }

  FilterConnection (const FilterConnection& other)
    : sourceFilterID (other.sourceFilterID),
      sourceChannel (other.sourceChannel),
      destFilterID (other.destFilterID),
      destChannel (other.destChannel),
      owner (other.owner)
  {
  }

  ~FilterConnection()
  {
  }

  uint32 sourceFilterID;
  int sourceChannel;
  uint32 destFilterID;
  int destChannel;
  juce_UseDebuggingNewOperator
  private:
  FilterGraph& owner;
  FilterConnection& operator= (const FilterConnection&);
};

//==============================================================================
// FilterGraph
//==============================================================================

/** A collection of filters and some connections between them. */

class FilterGraph   : public FileBasedDocument
{

public:

  FilterGraph();
  ~FilterGraph();
  AudioProcessorGraph& getGraph() 
  {
    return graph;
  }
  int getNumFilters() ;
  const AudioProcessorGraph::Node::Ptr getNode (const int index) ;
  const AudioProcessorGraph::Node::Ptr getNodeForId (const uint32 uid) ;
  void addFilter (const PluginDescription* desc, double x, double y);
  void removeFilter (const uint32 filterUID);
  void disconnectFilter (const uint32 filterUID);
  void removeIllegalConnections();
  void setNodePosition (const int nodeId, double x, double y);
  void getNodePosition (const int nodeId, double& x, double& y) ;
  int getNumConnections() ;
  const AudioProcessorGraph::Connection* getConnection (const int index) ;
  const AudioProcessorGraph::Connection* getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel,
                                                               uint32 destFilterUID, int destFilterChannel) ;
  bool canConnect (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel) ;
  bool addConnection (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel);
  void removeConnection (const int index);
  void removeConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                         uint32 destFilterUID, int destFilterChannel);

  void clear();
  XmlElement* createXml() const;
  void restoreFromXml (const XmlElement& xml);

  const String getDocumentTitle();
  const String loadDocument (const File& file);
  const String saveDocument (const File& file);
  const File getLastDocumentOpened();
  void setLastDocumentOpened (const File& file);

  /** The special channel index used to refer to a filter's midi channel.
   */
  static const int midiChannelNumber;
  AudioProcessorGraph graph;
  AudioProcessorPlayer player;

private:


  uint32 lastUID;
  uint32 getNextUID() ;

  void createNodeFromXml (const XmlElement& xml);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterGraph);
};

#endif

/*=======================================================================*
  Copyright (C) 2008 Rick Taube.                                        
  This program is free software; you can redistribute it and/or modify  
  it under the terms of the Lisp Lesser Gnu Public License. The text of 
  this agreement is available at http://www.cliki.net/LLGPL             
 *=======================================================================*/

#include "Enumerations.h"
#include "Preferences.h"
juce_ImplementSingleton (Preferences);

// comment= old: firebrick  new: #ce001a
// string=  old: rosybrown  new: #a11153

static char colorthemedata [] = "<colorthemes>\
<colortheme name=\"Clarity and Beauty\"\
            plaintext=\"#aac5e0\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#00ffff\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"lightsalmon\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"yellow\" background=\"black\"/>\
<colortheme name=\"Deep Blue\"\
            plaintext=\"#eeeeee\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#950083\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"#deb887\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"#00ff00\" background=\"#102e4e\"/>\
<colortheme name=\"Gnome\"\
            plaintext=\"#f5deb3\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#950083\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"lightsalmon\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"#d3d3d3\" background=\"#2f4f4f\"/>\
<colortheme name=\"Snowish\"\
            plaintext=\"#2f4f4f\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#950083\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"#9400d3\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"#cd0000\" background=\"#eee9e9\"/>\
<colortheme name=\"Emacs\"\
            plaintext=\"black\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#950083\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"rosybrown\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"black\" background=\"white\"/>\
<colortheme name=\"XEmacs\"\
            plaintext=\"black\" comment=\"#ce001a\" string=\"rosybrown\"\
            keyword1=\"cadetblue\" keyword2=\"#8a2f8f\" keyword3=\"orchid\"\
            literal1=\"#950083\" literal2=\"forestgreen\" literal3=\"blue\"\
            values=\"#00cd00\" output=\"#008b00\" warning=\"darkorange\"\
            error=\"#cd0000\" cursor=\"#cd0000\" background=\"#cccccc\"/>\
</colorthemes>";

Preferences::Preferences()
  : colorThemes(0),
    props (0)
{
  juce::PropertiesFile::Options options;
  options.applicationName=JUCEApplication::getInstance()->getApplicationName();
  options.filenameSuffix=T("xml");
  if (SysInfo::isMac())
    options.osxLibrarySubFolder=T("Library/Application Support");
  options.commonToAllUsers=false;
  options.millisecondsBeforeSaving=3000;
  options.storageFormat=PropertiesFile::storeAsXML;
  props=new juce::PropertiesFile(options);

  /*  props=PropertiesFile::createDefaultAppPropertiesFile
      (JUCEApplication::getInstance()->getApplicationName(),
      T("xml"), String::empty, false, 3000, PropertiesFile::storeAsXML);*/

  recentlyOpened.setMaxNumberOfItems(16);
  recentlyOpened.restoreFromString(props->getValue(T("RecentlyOpenedFiles")));
  recentlyOpened.removeNonExistentFiles();
  recentlyLoaded.setMaxNumberOfItems(8);
  recentlyLoaded.restoreFromString(props->getValue(T("RecentlyLoadedFiles")));
  recentlyLoaded.removeNonExistentFiles();
  recentlyLoadedGraphs.setMaxNumberOfItems(8);
  recentlyLoadedGraphs.restoreFromString(props->getValue(T("RecentlyLoadedGraphs")));
  recentlyLoadedGraphs.removeNonExistentFiles();

  String working=props->getValue(T("WorkingDirectory"));
  if (working.isNotEmpty())
    File(working).setAsCurrentWorkingDirectory();
  else
    if (SysInfo::isWindows())
      File::getSpecialLocation(File::userDesktopDirectory).setAsCurrentWorkingDirectory();
    else
      File::getSpecialLocation(File::userHomeDirectory).setAsCurrentWorkingDirectory();
  String str=String(colorthemedata);
  XmlDocument xmldoc (str);
  colorThemes=xmldoc.getDocumentElement();
}

int Preferences::numColorThemes()
{
  if (colorThemes) return colorThemes->getNumChildElements();
  return 0;
}

XmlElement* Preferences::getColorTheme(int i)
{
  if (colorThemes) return colorThemes->getChildElement(i);
  return 0;
}

XmlElement* Preferences::getColorTheme(const String name)
{
  for (int i=0; i<numColorThemes(); i++)
    {
      XmlElement* e=getColorTheme(i);
      if (name.equalsIgnoreCase(ColorThemeIDs::getColorThemeName(e)))
        return e;
    }
  return 0;
}

Preferences::~Preferences()
{
  flush();
  deleteAndZero (colorThemes);
  deleteAndZero (props);
  clearSingletonInstance();
}

PropertiesFile& Preferences::getProps()
{
  return *props;
}

void Preferences::flush()
{
  // flush exiting
  if (props != 0) 
    {
      props->setValue(T("RecentlyOpenedFiles"), recentlyOpened.toString());
      props->setValue(T("RecentlyLoadedFiles"), recentlyLoaded.toString());
      props->setValue(T("RecentlyLoadedGraphs"), recentlyLoadedGraphs.toString());
      props->setValue(T("WorkingDirectory"), 
		      File::getCurrentWorkingDirectory().getFullPathName());
    }
}

// Bool Props

bool Preferences::getBoolProp(String name, bool def)
{
  return props->getBoolValue(name, def);
}

void Preferences::setBoolProp(String name, bool val)
{
  props->setValue(name, val);
}

// Int Props

int Preferences::getIntProp(String name, int def)
{
  return props->getIntValue(name, def);
}

void Preferences::setIntProp(String name, int val)
{
  props->setValue(name, val);
}

// String Props

double Preferences::getDoubleProp(String name, double def)
{
  return props->getDoubleValue(name, def);
}

void Preferences::setDoubleProp(String name, double val)
{
  props->setValue(name, val);
}

// String Props

String Preferences::getStringProp(String name, String def)
{
  return props->getValue(name, def);
}


void Preferences::setStringProp(String name, String val)
{
  props->setValue(name, val);
}

// XML Props

XmlElement* Preferences::getXmlProp(String name, XmlElement* def)
{
  if (XmlElement* val=props->getXmlValue(name))
    return val;
  else
    return def;
}

void Preferences::setXmlProp(String name, XmlElement* val)
{
  props->setValue(name, val);
}

void Preferences::removeProp(String name)
{
  return props->removeValue(name);
}

// -*- C++ -*-

//=============================================================================
/**
*  @file    xml.hpp
*
*  definitions for internet operations
*
*  $Id: xml.hpp 85015 2013-09-25 12:27:59Z ssr $
*
*  @author sr shao <343206508@qq.com>
*/
//=============================================================================
#ifndef DM_XML_INCLUDE_H
#define DM_XML_INCLUDE_H
#include <string>
#include "tinyxml.h"

class DMXmlImpl;
class DMXml
{
public:
	DMXml();
	~DMXml();

	//ÐÂ½¨xml
	DMXml*	NewRoot(char const* name);

	DMXml*	NewChild(char const* name,const char*);
	DMXml*	GetRoot();
	DMXml*	GetParent();
	DMXml*	SetTextAttribute(char const*name, char const*attr);
	DMXml*	SetIntAttribute(char const*name,int attr) ;
	DMXml*	SetDoubleAttribute(char const*name,double attr);
	char*	Encode();
	//½âÎöxml

	void	Decode(char const*);

	DMXml* FindElement(char const*name);
	DMXml* FindAllElement(char const*name);
	char*	GetValueText();
	int		GetValueInt();
	double   GetValueDouble();
	char*	GetAttributeText(const char*name);
	int     GetAttributeInt(const char*name);
	double  GetAttributeDouble( const char*name);
private:
	TiXmlNode* m_pnode;
	DMXmlImpl* m_pxml;
};

//class DMXml
//{
//
//};


#endif
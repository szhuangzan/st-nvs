#include "xml.hpp"
#include "tinyxml.h"
#include "xml.hpp"
#include "tinystr.h"
#include <cstring>

class DMXmlImpl
{
public:
	DMXmlImpl();
	~DMXmlImpl();

	//ÐÂ½¨xmlm
	TiXmlNode*	DMXmlImpl_NewRoot(char const* name);
	TiXmlNode*	DMXmlImpl_NewChild(TiXmlNode*,char const* name,TiXmlText*);
	TiXmlNode*	DMXmlImpl_GetRoot();
	TiXmlNode*	DMXmlImpl_GetParent(TiXmlNode*node);

	void		DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name, char const*attr);
	void		DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name,int attr) ;
	void		DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name,double attr);
	char*		DMXmlImpl_Encode();
	//½âÎöxml

	void	DMXmlImpl_Decode(char const*);

	TiXmlElement* DMXmlImpl_FindElement(TiXmlNode*node,char const*name);
	TiXmlElement* DMXmlImpl_FindAllElement(TiXmlNode*node,char const*name);
	char*	DMXmlImpl_GetValueText(TiXmlNode*node);
	int		DMXmlImpl_GetValueInt(TiXmlNode*node);
	double   DMXmlImpl_GetValueDouble(TiXmlNode*node);
	char*	DMXmlImpl_GetAttributeText(TiXmlNode*node, const char*name);
	int     DMXmlImpl_GetAttributeInt(TiXmlNode*node, const char*name);
	double  DMXmlImpl_GetAttributeDouble(TiXmlNode*node, const char*name);
private:
	TiXmlDocument* m_Doc;
};
DMXmlImpl::DMXmlImpl()
:m_Doc(new TiXmlDocument)
{
	
}

DMXmlImpl::~DMXmlImpl()
{
	m_Doc->Clear();
	delete m_Doc;
}

TiXmlNode* DMXmlImpl::DMXmlImpl_NewRoot(char const* name)
{
	m_Doc->Clear();
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8","no");
	m_Doc->LinkEndChild(decl);
	TiXmlElement* n = new TiXmlElement(name);
	m_Doc->LinkEndChild(n);
	return (TiXmlNode*)n;
}

TiXmlNode* DMXmlImpl::DMXmlImpl_NewChild(TiXmlNode* node,char const* name,TiXmlText* text)
{
	TiXmlElement* nn = new TiXmlElement(name);
	if(text)
		nn->LinkEndChild((TiXmlNode*)text);
	node->LinkEndChild(nn);
	return (TiXmlNode*)nn;
}

void DMXmlImpl::DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name, char const*attr)
{
	TiXmlElement* ele = dynamic_cast<TiXmlElement*>(node);
	ele->SetAttribute(name,attr);
}

void DMXmlImpl::DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name,int attr)
{
	TiXmlElement* ele = dynamic_cast<TiXmlElement*>(node);
	ele->SetAttribute(name,attr);
}

void DMXmlImpl::DMXmlImpl_SetAttribute(TiXmlNode*node, char const*name,double attr)
{
	TiXmlElement* ele = dynamic_cast<TiXmlElement*>(node);
	ele->SetDoubleAttribute(name,attr);
}


TiXmlNode* DMXmlImpl::DMXmlImpl_GetRoot()
{
	return (TiXmlNode*)m_Doc->RootElement();
}

TiXmlNode* DMXmlImpl::DMXmlImpl_GetParent(TiXmlNode*node)
{
	TiXmlElement* ele = (TiXmlElement*)node;
	return (TiXmlNode*)ele->Parent();
}

char*DMXmlImpl::DMXmlImpl_Encode()
{
	TiXmlPrinter printer; 
	m_Doc->Accept(&printer);
	return strdup(printer.CStr());
}

void DMXmlImpl::DMXmlImpl_Decode(char const*context)
{
	m_Doc->Clear();
	m_Doc->Parse(context);
}

TiXmlElement* DMXmlImpl::DMXmlImpl_FindElement(TiXmlNode*node, char const*name)
{
	return (TiXmlElement*)node->FirstChild(name);

}

TiXmlElement* DMXmlImpl::DMXmlImpl_FindAllElement(TiXmlNode*node, char const*name)
{
	return (TiXmlElement*)node->NextSibling(name);
	
}

char* DMXmlImpl::DMXmlImpl_GetValueText(TiXmlNode*node)
{
	return strdup(dynamic_cast<TiXmlElement*>(node)->GetText());
}

int DMXmlImpl::DMXmlImpl_GetValueInt(TiXmlNode*node)
{
	return atol(dynamic_cast<TiXmlElement*>(node)->GetText());
}

double DMXmlImpl::DMXmlImpl_GetValueDouble(TiXmlNode*node)
{
	return atof(dynamic_cast<TiXmlElement*>(node)->GetText());
}


int DMXmlImpl::DMXmlImpl_GetAttributeInt(TiXmlNode*node, const char*name)
{
	int d = 0;
	dynamic_cast<TiXmlElement*>(node)->Attribute(name, &d);
	return d;
}

char * DMXmlImpl::DMXmlImpl_GetAttributeText(TiXmlNode*node, const char*name)
{
	return strdup(dynamic_cast<TiXmlElement*>(node)->Attribute(name));
}
double DMXmlImpl::DMXmlImpl_GetAttributeDouble(TiXmlNode*node, const char*name)
{
	double d = 0.0;
	strdup(dynamic_cast<TiXmlElement*>(node)->Attribute(name, &d));
	return d;
}

 DMXml::DMXml()
	 :m_pxml(new DMXmlImpl)
{
	
}
 DMXml::~DMXml()
 {
	delete  m_pxml;
 }

 DMXml* DMXml::NewRoot(char const* name)
 {
	 m_pnode = m_pxml->DMXmlImpl_NewRoot(name);
	 return this;
 }

 DMXml* DMXml::NewChild(char const* name,const char*value)
 {
	 if(value)
	 {
		 TiXmlText* text =new TiXmlText(value);
		 m_pnode = m_pxml->DMXmlImpl_NewChild(m_pnode,name, text);
	 }
	 else
		m_pnode = m_pxml->DMXmlImpl_NewChild(m_pnode,name, 0);
	 return this;
 }

 DMXml*DMXml::GetRoot()
 {
	 m_pnode = m_pxml->DMXmlImpl_GetRoot();
	 return this;
 }

 DMXml* DMXml::GetParent()
 {
	m_pnode = m_pxml->DMXmlImpl_GetParent(m_pnode);
	return this;
 }

 DMXml* DMXml::SetTextAttribute(char const*name,const char *attr)
 {
	 m_pxml->DMXmlImpl_SetAttribute(m_pnode,name,attr);
	 return this;
 } 
 
 DMXml* DMXml::SetDoubleAttribute(char const*name,double attr)
 {
	 m_pxml->DMXmlImpl_SetAttribute(m_pnode,name,attr);
	 return this;
 }

 DMXml* DMXml::SetIntAttribute(char const*name,int attr)
 {
	 m_pxml->DMXmlImpl_SetAttribute(m_pnode,name,attr);
	 return this;
 }

 char* DMXml::Encode()
 {
	return m_pxml->DMXmlImpl_Encode();
 }

 void DMXml::Decode(const char *xml)
 {
	 m_pxml->DMXmlImpl_Decode(xml);
 }

 DMXml* DMXml::FindElement(char const*name)
 {
	 if(m_pnode)
		m_pnode = dynamic_cast<TiXmlNode*>(m_pxml->DMXmlImpl_FindElement(m_pnode,name));
	return this;
 }

 DMXml* DMXml::FindAllElement(char const*name)
 {
	 if(m_pnode)
		 m_pnode = dynamic_cast<TiXmlNode*>(m_pxml->DMXmlImpl_FindAllElement(m_pnode,name));
	 return this;
 }


 char* DMXml::GetValueText()
 {
	if(!m_pnode)
		return 0;
	return m_pxml->DMXmlImpl_GetValueText(m_pnode);
 }
int DMXml::GetValueInt()
 {
	 if(!m_pnode)
		 return 0;
	 return m_pxml->DMXmlImpl_GetValueInt(m_pnode);
 }

 char*	DMXml::GetAttributeText(const char*name)
 {
	 if(!m_pnode)
		 return 0;
	 return m_pxml->DMXmlImpl_GetAttributeText(m_pnode, name);
 }
 int  DMXml::GetAttributeInt(const char*name)
 {
	 if(!m_pnode)
		 return -1;
	 return m_pxml->DMXmlImpl_GetAttributeInt(m_pnode, name);
 }
 double  DMXml::GetAttributeDouble( const char*name)
 {
	 if(!m_pnode)
		 return -1;
	 return m_pxml->DMXmlImpl_GetAttributeInt(m_pnode, name);
 }
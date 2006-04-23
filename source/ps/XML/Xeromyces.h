/*
  Xeromyces file-loading interface.
  Automatically creates and caches relatively
  efficient binary representations of XML files.

  - Philip Taylor (philip@zaynar.demon.co.uk / @wildfiregames.com)
*/

#ifndef _XEROMYCES_H_
#define _XEROMYCES_H_

#include "ps/Errors.h"
ERROR_GROUP(Xeromyces);
ERROR_TYPE(Xeromyces, XMLOpenFailed);
ERROR_TYPE(Xeromyces, XMLParseError);

#include "XeroXMB.h"
#include "CVFSFile.h"

class CXeromyces : public XMBFile
{
public:
	CXeromyces();
	~CXeromyces();

	// Load from an XML file (with invisible XMB caching).
	PSRETURN Load(const char* filename);

	// Call once when shutting down the program, to unload Xerces.
	static void Terminate();

private:
	bool ReadXMBFile(const char* filename);

	XMBFile* XMB;
	CVFSFile* XMBFileHandle; // if it's being read from disk
	char* XMBBuffer; // if it's being read from RAM

	static int XercesLoaded; // for once-only initialisation
};


#define _XERO_MAKE_UID2__(p,l) p ## l
#define _XERO_MAKE_UID1__(p,l) _XERO_MAKE_UID2__(p,l)

#define _XERO_CHILDREN _XERO_MAKE_UID1__(_children_, __LINE__)
#define _XERO_I _XERO_MAKE_UID1__(_i_, __LINE__)

#define XERO_ITER_EL(parent_element, child_element)					\
	XMBElementList _XERO_CHILDREN = parent_element.getChildNodes();	\
	XMBElement child_element (0);									\
	for (int _XERO_I = 0;											\
		 _XERO_I < _XERO_CHILDREN.Count								\
			&& (child_element = _XERO_CHILDREN.item(_XERO_I), 1);	\
		 ++_XERO_I)

#define XERO_ITER_ATTR(parent_element, attribute)						\
	XMBAttributeList _XERO_CHILDREN = parent_element.getAttributes();	\
	XMBAttribute attribute;												\
	for (int _XERO_I = 0;												\
		 _XERO_I < _XERO_CHILDREN.Count									\
			&& (attribute = _XERO_CHILDREN.item(_XERO_I), 1);			\
		 ++_XERO_I)

#endif // _XEROMYCES_H_

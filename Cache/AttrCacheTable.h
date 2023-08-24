#ifndef NITCBASE_ATTRCACHETABLE_H
#define NITCBASE_ATTRCACHETABLE_H

#include "../Buffer/BlockBuffer.h"
#include "../define/constants.h"
#include "../define/id.h"
#include <cstring>
#include <string>
#include <list>

typedef struct AttrCatEntry {
	char relName[ ATTR_SIZE ];
	char attrName[ ATTR_SIZE ];
	int attrType;
	bool primaryFlag;
	int rootBlock;
	int offset;
	const char* getTypeString( ) {
		if ( attrType == NUMBER )
			return "NUM";
		else
			return "STR";
	}
	AttrCatEntry() {
		std::strcpy(relName, "");
		std::strcpy(attrName, "");
		attrType=-1;
		primaryFlag =0;
		rootBlock = -1;
	}

} AttrCatEntry;

typedef struct AttrCacheEntry {
	AttrCatEntry attrCatEntry;
	bool dirty;
	RecId recId;
	IndexId searchIndex;
	struct AttrCacheEntry* next;

} AttrCacheEntry;

class AttrCacheTable {
	friend class OpenRelTable;

  public:
	// methods
	static int getAttrCatEntry( int relId, char attrName[ ATTR_SIZE ], std::unique_ptr<AttrCatEntry>& attrCatBuf );
	static int getAttrCatEntry( int relId, int attrOffset, std::unique_ptr<AttrCatEntry>& attrCatBuf );
	static int setAttrCatEntry( int relId, char attrName[ ATTR_SIZE ], std::unique_ptr<AttrCatEntry>& attrCatBuf );
	static int setAttrCatEntry( int relId, int attrOffset, std::unique_ptr<AttrCatEntry>& attrCatBuf );
	static int getSearchIndex( int relId, char attrName[ ATTR_SIZE ], IndexId* searchIndex );
	static int getSearchIndex( int relId, int attrOffset, IndexId* searchIndex );
	static int setSearchIndex( int relId, char attrName[ ATTR_SIZE ], IndexId* searchIndex );
	static int setSearchIndex( int relId, int attrOffset, IndexId* searchIndex );
	static int resetSearchIndex( int relId, char attrName[ ATTR_SIZE ] );
	static int resetSearchIndex( int relId, int attrOffset );

  private:
	// field
	// static AttrCacheEntry* attrCache[ MAX_OPEN ];
	static std::list<AttrCacheEntry> attrCache[ MAX_OPEN];

	// methods
	static void recordToAttrCatEntry( union Attribute record[ ATTRCAT_NO_ATTRS ], std::unique_ptr<AttrCatEntry>& attrCatEntry );
	static void attrCatEntryToRecord( std::unique_ptr<AttrCatEntry>& attrCatEntry, union Attribute record[ ATTRCAT_NO_ATTRS ] );
};

#endif // NITCBASE_ATTRCACHETABLE_H

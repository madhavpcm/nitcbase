#include "AttrCacheTable.h"
#include <cstring>
// AttrCacheEntry* AttrCacheTable::attrCache[ MAX_OPEN ];
std::list<AttrCacheEntry> AttrCacheTable::attrCache[ MAX_OPEN ];

/* returns the attrOffset-th attribute for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
@param relId:
*/
int AttrCacheTable::getAttrCatEntry( int relId, int attrOffset, AttrCatEntry* attrCatBuf ) {
	// check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
	if ( relId < 0 || relId > MAX_OPEN )
		return E_OUTOFBOUND;

	// check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true
	if ( attrCache[ relId ].size( ) == 0 )
		return E_RELNOTOPEN;

	// traverse the linked list of attribute cache entries
	std::list<AttrCacheEntry>::iterator entry;
	for ( entry = attrCache[ relId ].begin( ); entry != attrCache[ relId ].end( ); entry++ ) {
		if ( entry->attrCatEntry.offset == attrOffset ) {
			// copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
			*attrCatBuf = entry->attrCatEntry;
			return SUCCESS;
		}
	}

	// there is no attribute at this offset
	return E_ATTRNOTEXIST;
}

/* Converts a attribute catalog record to AttrCatEntry
 * We get the record as Attribute[] from the BlockBuffer.getRecord() function.
 This function will convert that to a struct AttrCatEntry type.
*/
void AttrCacheTable::recordToAttrCatEntry(
	union Attribute record[ ATTRCAT_NO_ATTRS ], AttrCatEntry* attrCatEntry ) {
	// copy the rest of the fields in the record to the attrCacheEntry struct
	std::copy( record[ ATTRCAT_REL_NAME_INDEX ].sVal, record[ ATTRCAT_REL_NAME_INDEX ].sVal + ATTR_SIZE,
		attrCatEntry->relName );
	attrCatEntry->offset = ( int )record[ ATTRCAT_OFFSET_INDEX ].nVal;
	std::copy( record[ ATTRCAT_ATTR_NAME_INDEX ].sVal, record[ ATTRCAT_ATTR_NAME_INDEX ].sVal + ATTR_SIZE,
		attrCatEntry->attrName );
	attrCatEntry->attrType	  = ( int )record[ ATTRCAT_ATTR_TYPE_INDEX ].nVal;
	attrCatEntry->rootBlock	  = ( int )record[ ATTRCAT_ROOT_BLOCK_INDEX ].nVal;
	attrCatEntry->primaryFlag = ( int )record[ ATTRCAT_PRIMARY_FLAG_INDEX ].nVal;
}

/* returns the attribute with name `attrName` for the relation corresponding to
relId NOTE: this function expects the caller to allocate memory for
`*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(
	int relId, char attrName[ ATTR_SIZE ], AttrCatEntry* attrCatBuf ) {

	// check that relId is valid and corresponds to an open relation
	if ( relId < 0 || relId > MAX_OPEN )
		return E_OUTOFBOUND;

	// check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true
	if ( attrCache[ relId ].size( ) == 0 )
		return E_RELNOTOPEN;

	// traverse the linked list of attribute cache entries
	std::list<AttrCacheEntry>::iterator entry;
	for ( entry = attrCache[ relId ].begin( ); entry != attrCache[ relId ].end( ); entry++ ) {
		if ( std::strcmp( attrName, entry->attrCatEntry.attrName ) == 0 ) {
			// copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
			*attrCatBuf = entry->attrCatEntry;
			return SUCCESS;
		}
	}

	return E_ATTRNOTEXIST;
}

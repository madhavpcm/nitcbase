#include "RelCacheTable.h"
#include <cstring>
#include <memory>

std::array<RelCacheEntry*, MAX_OPEN> RelCacheTable::relCache;

/*
Get the relation catalog entry for the relation with rel-id `relId` from the
cache NOTE: this function expects the caller to allocate memory for `*relCatBuf`
*/
int RelCacheTable::getRelCatEntry( int relId, std::unique_ptr<RelCatEntry>& relCatBuf ) {
	if ( relId < 0 || relId >= MAX_OPEN ) {
		return E_OUTOFBOUND;
	}
	if ( relCache[ relId ] == nullptr ) {
		return E_RELNOTOPEN;
	}
	*relCatBuf = relCache[ relId ]->relCatEntry;
	return SUCCESS;
}

/* Converts a relation catalog record to RelCatEntry struct
								We get the record as Attribute[] from the
BlockBuffer.getRecord() function. This function will convert that to a struct
RelCatEntry type. NOTE: this function expects the caller to allocate memory for
`*relCatEntry`
*/

void RelCacheTable::recordToRelCatEntry( union Attribute record[ RELCAT_NO_ATTRS ], RelCatEntry* relCatEntry ) {
	std::copy(
		record[ RELCAT_REL_NAME_INDEX ].sVal, record[ RELCAT_REL_NAME_INDEX ].sVal + ATTR_SIZE, relCatEntry->relName );
	relCatEntry->numAttrs		= ( int )record[ RELCAT_NO_ATTRIBUTES_INDEX ].nVal;
	relCatEntry->numRecs		= ( int )record[ RELCAT_NO_RECORDS_INDEX ].nVal;
	relCatEntry->firstBlk		= ( int )record[ RELCAT_FIRST_BLOCK_INDEX ].nVal;
	relCatEntry->lastBlk		= ( int )record[ RELCAT_LAST_BLOCK_INDEX ].nVal;
	relCatEntry->numSlotsPerBlk = ( int )record[ RELCAT_NO_SLOTS_PER_BLOCK_INDEX ].nVal;
}

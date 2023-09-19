#include "BlockAccess.h"
#include <cstring>
#include <vector>

RecId BlockAccess::linearSearch( int relId, char attrName[ ATTR_SIZE ], union Attribute attrVal, int op ) {
	// get the previous search index of the relation relId from the relation cache
	// (use RelCacheTable::getSearchIndex() function)

	std::unique_ptr<RecId> prevRecId = std::make_unique<RecId>( );
	if ( RelCacheTable::getSearchIndex( relId, prevRecId.get( ) ) != SUCCESS )
		return { -1, -1 };

	// let block and slot denote the record id of the record being currently
	// checked
	int block, slot;
	// if the current search index record is invalid(i.e. both block and slot
	// = -1)
	//
	block = slot = -1;
	if ( prevRecId->block == -1 && prevRecId->slot == -1 ) {
		// (no hits from previous search; search should start from the
		// first record itself)

		// get the first record block of the relation from the relation cache
		// (use RelCacheTable::getRelCatEntry() function of Cache Layer)

		// block = first record block of the relation
		// slot = 0
		std::unique_ptr<RelCatEntry> firstRecordBlockOfRel = std::make_unique<RelCatEntry>( );
		block = RelCacheTable::getRelCatEntry( relId, firstRecordBlockOfRel.get( ) ) == SUCCESS
					? firstRecordBlockOfRel.get( )->firstBlk
					: -1;
		slot  = 0;
	} else {
		// (there is a hit from previous search; search should start from
		// the record next to the search index record)

		// block = search index's block
		// slot = search index's slot + 1
		block = prevRecId->block;
		slot  = prevRecId->slot + 1;
	}

	/* The following code searches for the next record in the relation
	   that satisfies the given condition
	   We start from the record id (block, slot) and iterate over the remaining
	   records of the relation
	*/
	while ( block != -1 ) {
		/* create a RecBuffer object for block (use RecBuffer Constructor for
		   existing block) */

		RecBuffer blockBuffer( block );
		HeadInfo blockHeader;
		blockBuffer.getHeader( &blockHeader );

		std::unique_ptr<union Attribute[]> blkRecord( new union Attribute[ blockHeader.numAttrs ] );
		std::unique_ptr<unsigned char[]> blkSlotMap( new unsigned char[ SLOTMAPSIZE( blockHeader.numAttrs ) ] );
		// get the record with id (block, slot) using
		// RecBuffer::getRecord() get header of the block using
		// RecBuffer::getHeader() function get slot map of the block
		// using RecBuffer::getSlotMap() function

		blockBuffer.getRecord( blkRecord.get( ), slot );
		blockBuffer.getSlotMap( blkSlotMap.get( ) );

		// If slot >= the number of slots per block(i.e. no more slots
		// in this block)
		if ( slot >= blockHeader.numSlots ) {
			// update block = right block of block
			// update slot = 0
			block = blockHeader.rblock;
			slot  = 0;
			continue; // continue to the beginning of this while loop
		}

		// if slot is free skip the loop
		// (i.e. check if slot'th entry in slot map of block contains
		// SLOT_UNOCCUPIED)
		if ( blkSlotMap.get( )[ slot ] == SLOT_UNOCCUPIED ) {
			slot++;
			continue;
		}

		std::unique_ptr<AttrCatEntry> holder = std::make_unique<AttrCatEntry>( );

		// compare record's attribute value to the the given attrVal as
		// below:
		/*
		   the attribute offset for the attrName attribute from the
		   attribute cache entry of the relation using
		   AttrCacheTable::getAttrCatEntry()
		*/
		AttrCacheTable::getAttrCatEntry( relId, attrName, holder.get( ) );
		/* use the attribute offset to get the value of the attribute
		   from current record */
		union Attribute u;
		std::strcpy( u.sVal, blkRecord[ holder.get( )->offset ].sVal );
		int cmpVal = compareAttrs( static_cast<union Attribute*>( &blkRecord[ holder.get( )->offset ] ),
			static_cast<union Attribute*>( &attrVal ),
			holder.get( )->attrType ); // will store the difference between the attributes
		// set cmpVal using compareAttrs()

		/* Next task is to check whether this record satisfies the given
		   condition. It is determined based on the output of previous
		   comparison and the op value received. The following code sets
		   the cond variable if the condition is satisfied.
		*/
		if ( ( op == NE && cmpVal != 0 ) || // if op is "not equal to"
			 ( op == LT && cmpVal < 0 ) ||	// if op is "less than"
			 ( op == LE && cmpVal <= 0 ) || // if op is "less than or equal to"
			 ( op == EQ && cmpVal == 0 ) || // if op is "equal to"
			 ( op == GT && cmpVal > 0 ) ||	// if op is "greater than"
			 ( op == GE && cmpVal >= 0 )	// if op is "greater than or equal to"
		) {
			/*
			set the search index in the relation cache as
			the record id of the record that satisfies the given condition
			(use RelCacheTable::setSearchIndex function)
			*/
			RecId r{ block, slot };
			RelCacheTable::setSearchIndex( relId, &r );

			return r;
		}

		slot++;
	}

	// no record in the relation with Id relid satisfies the given condition
	return RecId{ -1, -1 };
}
int BlockAccess::renameRelation( char* oldName, char* newName ) {
	auto relId = OpenRelTable::getRelId( oldName );
	RelCacheTable::resetSearchIndex( relId );

	union Attribute newRelationName;
	std::strcpy( newRelationName.sVal, newName );
	char arr[ ATTR_SIZE ] = "RelName";

	/*
	 * Check if operation is valid
	 */
	auto newNameRelId = BlockAccess::linearSearch( RELCAT_RELID, arr, newRelationName, EQ );
	if ( newNameRelId != RecId{ -1, -1 } ) {
		return E_RELEXIST;
	}

	/*
	 * Update relation catalog
	 */
	RelCacheTable::resetSearchIndex( RELCAT_RELID );
	union Attribute oldRelationName;
	std::strcpy( oldRelationName.sVal, oldName );

	auto oldNameRelId = BlockAccess::linearSearch( RELCAT_RELID, arr, oldRelationName, EQ );
	if ( oldNameRelId == RecId{ -1, -1 } ) {
		return E_RELNOTEXIST;
	}

	RecBuffer relCatBuffer( RELCAT_BLOCK );
	std::array<union Attribute, RELCAT_NO_ATTRS> relCatRecord;
	relCatBuffer.getRecord( relCatRecord.data( ), oldNameRelId.slot );

	std::strcpy( relCatRecord[ RELCAT_REL_NAME_INDEX ].sVal, newName );
	relCatBuffer.setRecord( relCatRecord.data( ), oldNameRelId.slot );

	/*
	 * Update Attribute Catalog
	 */
	RelCacheTable::resetSearchIndex( ATTRCAT_RELID );

	std::array<union Attribute, ATTRCAT_NO_ATTRS> attrCatRecord;
	for ( int i = 0; i < relCatRecord[ RELCAT_NO_ATTRIBUTES_INDEX ].nVal; i++ ) {
		auto attrCatEntry = BlockAccess::linearSearch( ATTRCAT_RELID, arr, oldRelationName, EQ );
		RecBuffer attrCatBuffer( attrCatEntry.block );

		attrCatBuffer.getRecord( attrCatRecord.data( ), attrCatEntry.slot );
		std::strcpy( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, newName );
		attrCatBuffer.setRecord( attrCatRecord.data( ), attrCatEntry.slot );
	}

	return SUCCESS;
}

int BlockAccess::renameAttribute( char relName[ ATTR_SIZE ], char oldName[ ATTR_SIZE ], char newName[ ATTR_SIZE ] ) {

	/* reset the searchIndex of the relation catalog using
	   RelCacheTable::resetSearchIndex() */
	RelCacheTable::resetSearchIndex( RELCAT_RELID );

	union Attribute relationName;
	std::strcpy( relationName.sVal, relName );
	char arr[ ATTR_SIZE ]  = "RelName";
	char arr2[ ATTR_SIZE ] = "RelName";

	auto oldNameRelId = BlockAccess::linearSearch( RELCAT_RELID, arr, relationName, EQ );
	if ( oldNameRelId == RecId{ -1, -1 } ) {
		return E_RELNOTEXIST;
	}

	RelCacheTable::resetSearchIndex( ATTRCAT_RELID );

	/* declare variable attrToRenameRecId used to store the attr-cat recId
	of the attribute to rename */
	RecId attrToRenameRecId{ -1, -1 };
	std::array<Attribute, ATTRCAT_NO_ATTRS> attrCatEntryRecord;

	/* iterate over all Attribute Catalog Entry record corresponding to the
	   relation to find the required attribute */
	RecId targetAttr{ -1, -1 };
	while ( true ) {
		// linear search on the attribute catalog for RelName = relNameAttr
		auto res = BlockAccess::linearSearch( ATTRCAT_RELID, arr, relationName, EQ );

		// End of search
		if ( res == RecId{ -1, -1 } ) {
			break;
		}

		RecBuffer attrCatBuffer( res.block );
		attrCatBuffer.getRecord( attrCatEntryRecord.data( ), res.slot );

		// if this is the attribute we want to change
		if ( std::strcmp( attrCatEntryRecord[ ATTRCAT_ATTR_NAME_INDEX ].sVal, oldName ) == 0 )
			targetAttr = res;

		// if this attribute name already exists
		if ( std::strcmp( attrCatEntryRecord[ ATTRCAT_ATTR_NAME_INDEX ].sVal, newName ) == 0 )
			return E_ATTREXIST;
	}

	if ( targetAttr == RecId{ -1, -1 } )
		return E_ATTRNOTEXIST;

	RecBuffer targetBuffer( targetAttr.block );

	targetBuffer.getRecord( attrCatEntryRecord.data( ), targetAttr.slot );
	std::strcpy( attrCatEntryRecord[ ATTRCAT_ATTR_NAME_INDEX ].sVal, newName );
	targetBuffer.setRecord( attrCatEntryRecord.data( ), targetAttr.slot );

	return SUCCESS;
}

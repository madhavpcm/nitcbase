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

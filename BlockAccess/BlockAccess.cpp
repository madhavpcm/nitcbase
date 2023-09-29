#include "BlockAccess.h"
#include "../BPlusTree/BPlusTree.h"
#include "../Cache/OpenRelTable.h"
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
	assert_res( relCatBuffer.getRecord( relCatRecord.data( ), oldNameRelId.slot ), SUCCESS );

	std::strcpy( relCatRecord[ RELCAT_REL_NAME_INDEX ].sVal, newName );
	assert_res( relCatBuffer.setRecord( relCatRecord.data( ), oldNameRelId.slot ), SUCCESS );

	/*
	 * Update Attribute Catalog
	 */
	assert_res( RelCacheTable::resetSearchIndex( ATTRCAT_RELID ), SUCCESS );

	std::array<union Attribute, ATTRCAT_NO_ATTRS> attrCatRecord;
	for ( int i = 0; i < relCatRecord[ RELCAT_NO_ATTRIBUTES_INDEX ].nVal; i++ ) {
		auto attrCatEntry = BlockAccess::linearSearch( ATTRCAT_RELID, arr, oldRelationName, EQ );
		RecBuffer attrCatBuffer( attrCatEntry.block );

		assert_res( attrCatBuffer.getRecord( attrCatRecord.data( ), attrCatEntry.slot ), SUCCESS );
		std::strcpy( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, newName );
		assert_res( attrCatBuffer.setRecord( attrCatRecord.data( ), attrCatEntry.slot ), SUCCESS );
	}

	return SUCCESS;
}

int BlockAccess::renameAttribute( char relName[ ATTR_SIZE ], char oldName[ ATTR_SIZE ], char newName[ ATTR_SIZE ] ) {

	/* reset the searchIndex of the relation catalog using
	   RelCacheTable::resetSearchIndex() */
	assert_res( RelCacheTable::resetSearchIndex( RELCAT_RELID ), SUCCESS );

	union Attribute relationName;
	std::strcpy( relationName.sVal, relName );
	char arr[ ATTR_SIZE ]  = "RelName";
	char arr2[ ATTR_SIZE ] = "RelName";

	auto oldNameRelId = BlockAccess::linearSearch( RELCAT_RELID, arr, relationName, EQ );
	if ( oldNameRelId == RecId{ -1, -1 } ) {
		return E_RELNOTEXIST;
	}

	assert_res( RelCacheTable::resetSearchIndex( ATTRCAT_RELID ), SUCCESS );

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
		assert_res( attrCatBuffer.getRecord( attrCatEntryRecord.data( ), res.slot ), SUCCESS );

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

	assert_res( targetBuffer.getRecord( attrCatEntryRecord.data( ), targetAttr.slot ), SUCCESS );
	std::strcpy( attrCatEntryRecord[ ATTRCAT_ATTR_NAME_INDEX ].sVal, newName );
	assert_res( targetBuffer.setRecord( attrCatEntryRecord.data( ), targetAttr.slot ), SUCCESS );

	return SUCCESS;
}

int BlockAccess::insert( int relId, Attribute* record ) {
	// get the relation catalog entry from relation cache
	// ( use RelCacheTable::getRelCatEntry() of Cache Layer)
	RelCatEntry relCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( relId, &relCatEntry ), SUCCESS );

	int blockNum = relCatEntry.firstBlk;
	;

	// rec_id will be used to store where the new record will be inserted
	RecId rec_id = { -1, -1 };

	int numOfSlots		= relCatEntry.numSlotsPerBlk /* number of slots per record block */;
	int numOfAttributes = relCatEntry.numAttrs /* number of attributes of the relation */;

	int prevBlockNum = relCatEntry.lastBlk;
	while ( blockNum != -1 ) {
		// create a RecBuffer object for blockNum (using appropriate constructor!)
		RecBuffer recBuffer( blockNum );

		// get header of block(blockNum) using RecBuffer::getHeader() function
		HeadInfo header;
		assert_res( recBuffer.getHeader( &header ), SUCCESS );

		// get slot map of block(blockNum) using RecBuffer::getSlotMap() function
		std::unique_ptr<unsigned char[]> slotMapPtr( new unsigned char[ numOfSlots ] );
		assert_res( recBuffer.getSlotMap( slotMapPtr.get( ) ), SUCCESS );

		// search for free slot in the block 'blockNum' and store it's rec-id in
		// rec_id (Free slot can be found by iterating over the slot map of the
		// block)
		/* slot map stores SLOT_UNOCCUPIED if slot is free and
		   SLOT_OCCUPIED if slot is occupied) */
		int freeslot = -1;
		for ( int i = 0; i < numOfSlots; i++ ) {
			if ( *( slotMapPtr.get( ) + i ) == SLOT_UNOCCUPIED ) {
				freeslot = i;
				break;
			}
		}

		/* if a free slot is found, set rec_id and discontinue the traversal
		   of the linked list of record blocks (break from the loop) */
		if ( freeslot != -1 ) {
			rec_id = { blockNum, freeslot };
			break;
		}

		/* otherwise, continue to check the next block by updating the
		   block numbers as follows:
		   prevBlockNum = blockNum update blockNum = header.rblock (next element in
		   the linked list of record blocks)
		*/
		prevBlockNum = blockNum;
		blockNum	 = header.rblock;
	}

	//  if no free slot is found in existing record blocks (rec_id = {-1, -1})
	if ( rec_id == RecId{ -1, -1 } ) {
		// if relation is RELCAT, do not allocate any more blocks
		//     return E_MAXRELATIONS;
		if ( std::strcmp( relCatEntry.relName, "RELATIONCAT" ) == 0 ) {
			return E_MAXRELATIONS;
		}

		// Otherwise,
		// get a new record block (using the appropriate RecBuffer constructor!)
		// get the block number of the newly allocated block
		// (use BlockBuffer::getBlockNum() function)
		// let ret be the return value of getBlockNum() function call
		RecBuffer recBuffer;
		int ret = recBuffer.getBlockNum( );

		if ( ret == E_DISKFULL ) {
			return E_DISKFULL;
		}

		// Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
		rec_id = { ret, 0 };

		/*
		   header of the new record block such that it links with existing record
		   blocks of the relation set the block's header as follows: blockType: REC,
		   pblock: -1 lblock = -1 (if linked list of existing record blocks was
		   empty i.e this is the first insertion into the relation) = prevBlockNum
		   (otherwise), rblock: -1, numEntries: 0, numSlots: numOfSlots, numAttrs:
		   numOfAttributes (use BlockBuffer::setHeader() function)
		   TODO
		*/
		HeadInfo header{ REC, -1, prevBlockNum, -1, 0, numOfAttributes, numOfSlots };
		assert_res( recBuffer.setHeader( &header ), SUCCESS );

		/*
		   slot map with all slots marked as free (i.e. store SLOT_UNOCCUPIED for
		   all the entries) (use RecBuffer::setSlotMap() function)
		*/
		std::unique_ptr<unsigned char[]> slotMap( new unsigned char[ numOfSlots ] );
		for ( int i = 0; i < numOfSlots; i++ ) {
			slotMap[ i ] = SLOT_UNOCCUPIED;
		}
		assert_res( recBuffer.setSlotMap( slotMap.get( ) ), SUCCESS );

		// if prevBlockNum != -1
		if ( prevBlockNum != -1 ) {
			// create a RecBuffer object for prevBlockNum
			RecBuffer prevRecBuffer( prevBlockNum );
			// get the header of the block prevBlockNum and
			HeadInfo preHeader;
			assert_res( prevRecBuffer.getHeader( &preHeader ), SUCCESS );
			// update the rblock field of the header to the new block
			// number i.e. rec_id.block
			preHeader.rblock = rec_id.block;
			// (use BlockBuffer::setHeader() function)
			assert_res( prevRecBuffer.setHeader( &preHeader ), SUCCESS );
		} else {
			// update first block field in the relation catalog entry to the
			// new block (using RelCacheTable::setRelCatEntry() function)
			relCatEntry.firstBlk = rec_id.block;
		}

		// update last block field in the relation catalog entry to the
		// new block (using RelCacheTable::setRelCatEntry() function)
		relCatEntry.lastBlk = rec_id.block;
		assert_res( RelCacheTable::setRelCatEntry( relId, &relCatEntry ), SUCCESS );
	}

	// create a RecBuffer object for rec_id.block
	// insert the record into rec_id'th slot using RecBuffer.setRecord())
	RecBuffer recBuffer( rec_id.block );
	assert_res( recBuffer.setRecord( record, rec_id.slot ), SUCCESS );

	/* update the slot map of the block by marking entry of the slot to
	   which record was inserted as occupied) */
	std::unique_ptr<unsigned char[]> slotMap( new unsigned char[ numOfSlots ] );
	assert_res( recBuffer.getSlotMap( slotMap.get( ) ), SUCCESS );
	// (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
	// (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
	*( slotMap.get( ) + rec_id.slot ) = SLOT_OCCUPIED;
	assert_res( recBuffer.setSlotMap( slotMap.get( ) ), SUCCESS );

	// increment the numEntries field in the header of the block to
	// which record was inserted
	// (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
	HeadInfo header;
	assert_res( recBuffer.getHeader( &header ), SUCCESS );
	header.numEntries++;
	assert_res( recBuffer.setHeader( &header ), SUCCESS );

	// Increment the number of records field in the relation cache entry for
	// the relation. (use RelCacheTable::setRelCatEntry function)
	relCatEntry.numRecs++;
	assert_res( RelCacheTable::setRelCatEntry( relId, &relCatEntry ), SUCCESS );

	return SUCCESS;
}

int BlockAccess::deleteRelation( char relName[ ATTR_SIZE ] ) {
	// if the relation to delete is either Relation Catalog or Attribute Catalog,
	//     return E_NOTPERMITTED
	// (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
	// you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
	if ( std::strcmp( relName, RELCAT_RELNAME ) == 0 || std::strcmp( relName, ATTRCAT_RELNAME ) == 0 )
		return E_NOTPERMITTED;

	/* reset the searchIndex of the relation catalog using
	   RelCacheTable::resetSearchIndex() */
	RelCacheTable::resetSearchIndex( RELCAT_RELID );

	Attribute relNameAttr; // (stores relName as type union Attribute)
	std::strcpy( relNameAttr.sVal, relName );
	char relcat_relname[] = "RelName";

	//  linearSearch on the relation catalog for RelName = relNameAttr
	auto recId = BlockAccess::linearSearch( RELCAT_RELID, relcat_relname, relNameAttr, EQ );

	// if the relation does not exist (linearSearch returned {-1, -1})
	//     return E_RELNOTEXIST
	if ( recId == RecId{ -1, -1 } )
		return E_RELNOTEXIST;

	std::array<Attribute, RELCAT_NO_ATTRS> relCatEntryRecord;
	/* store the relation catalog record corresponding to the relation in
	   relCatEntryRecord using RecBuffer.getRecord */
	RecBuffer relCatBuffer( recId.block );
	assert_res( relCatBuffer.getRecord( relCatEntryRecord.data( ), recId.slot ), SUCCESS );

	/* get the first record block of the relation (firstBlock) using the
	   relation catalog entry record */
	int firstBlock = relCatEntryRecord[ RELCAT_FIRST_BLOCK_INDEX ].nVal;
	/* get the number of attributes corresponding to the relation (numAttrs)
	   using the relation catalog entry record */
	int numAttrs = relCatEntryRecord[ RELCAT_NO_ATTRIBUTES_INDEX ].nVal;

	/*
	 Delete all the record blocks of the relation
	*/
	// for each record block of the relation:
	int blk = firstBlock;
	while ( blk != -1 ) {
		RecBuffer currBuffer( blk );
		HeadInfo currHeader;
		//     get block header using BlockBuffer.getHeader
		assert_res( currBuffer.getHeader( &currHeader ), SUCCESS );
		//     get the next block from the header (rblock)
		auto nextBlk = currHeader.rblock;
		//     release the block using BlockBuffer.releaseBlock
		currBuffer.releaseBlock( );
		blk = nextBlk;
	}

	/***
	 * Deleting attribute catalog entries corresponding the relation
	 * and index blocks corresponding to the relation with relName on its
	 *attributes
	 ***/

	// reset the searchIndex of the attribute catalog
	RelCacheTable::resetSearchIndex( ATTRCAT_RELID );

	int numberOfAttributesDeleted = 0;

	while ( true ) {
		RecId attrCatRecId;
		// attrCatRecId = linearSearch on attribute catalog for RelName =
		// relNameAttr
		attrCatRecId = BlockAccess::linearSearch( ATTRCAT_RELID, relcat_relname, relNameAttr, EQ );

		// if no more attributes to iterate over (attrCatRecId == {-1, -1})
		//     break;
		if ( attrCatRecId == RecId{ -1, -1 } )
			break;

		numberOfAttributesDeleted++;

		// create a RecBuffer for attrCatRecId.block
		RecBuffer attrBuffer( attrCatRecId.block );
		// get the header of the block
		HeadInfo attrHeader;
		assert_res( attrBuffer.getHeader( &attrHeader ), SUCCESS );
		// get the record corresponding to attrCatRecId.slot
		std::array<union Attribute, ATTRCAT_NO_ATTRS> attrRecord;
		assert_res( attrBuffer.getRecord( attrRecord.data( ), attrCatRecId.slot ), SUCCESS );

		// declare variable rootBlock which will be used to store the root
		// block field from the attribute catalog record.
		int rootBlock = attrRecord[ ATTRCAT_ROOT_BLOCK_INDEX ].nVal /* get root block from the record */;
		// (This will be used later to delete any indexes if it exists)

		// Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
		// Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
		std::unique_ptr<unsigned char[]> attrSlotMapPtr( new unsigned char[ attrHeader.numSlots ] );
		assert_res( attrBuffer.getSlotMap( attrSlotMapPtr.get( ) ), SUCCESS );
		*( attrSlotMapPtr.get( ) + attrCatRecId.slot ) = SLOT_UNOCCUPIED;
		assert_res( attrBuffer.setSlotMap( attrSlotMapPtr.get( ) ), SUCCESS );

		/*
		   Decrement the numEntries in the header of the block corresponding to
		   the attribute catalog entry and then set back the header
		   using RecBuffer.setHeader
		*/
		attrHeader.numEntries--;
		assert_res( attrBuffer.setHeader( &attrHeader ), SUCCESS );

		/* If number of entries become 0, releaseBlock is called after fixing
		   the linked list.
		*/
		if ( attrHeader.numEntries == 0 ) {
			/* Standard Linked List Delete for a Block
			   Get the header of the left block and set it's rblock to this
			   block's rblock
			*/

			// create a RecBuffer for lblock and call appropriate methods
			int leftBlk = attrHeader.lblock;
			RecBuffer leftBuffer( leftBlk );
			HeadInfo leftHeader;
			assert_res( leftBuffer.getHeader( &leftHeader ), SUCCESS );
			leftHeader.rblock = attrHeader.rblock;
			assert_res( leftBuffer.setHeader( &leftHeader ), SUCCESS );

			if ( attrHeader.rblock != -1 /* header.rblock != -1 */ ) {
				/* Get the header of the right block and set it's lblock to
				   this block's lblock */
				// create a RecBuffer for rblock and call appropriate methods
				int rightBlk = attrHeader.rblock;
				RecBuffer rightBuffer( rightBlk );
				HeadInfo rightHeader;
				assert_res( rightBuffer.getHeader( &rightHeader ), SUCCESS );
				rightHeader.lblock = attrHeader.lblock;
				assert_res( rightBuffer.setHeader( &rightHeader ), SUCCESS );

			} else {
				// (the block being released is the "Last Block" of the relation.)
				/* update the Relation Catalog entry's LastBlock field for this
				   relation with the block number of the previous block. */
				RelCatEntry relCatAttrCatEntry;
				assert_res( RelCacheTable::getRelCatEntry( ATTRCAT_RELID, &relCatAttrCatEntry ), SUCCESS );
				relCatAttrCatEntry.lastBlk = attrHeader.lblock;
				assert_res( RelCacheTable::setRelCatEntry( ATTRCAT_RELID, &relCatAttrCatEntry ), SUCCESS );
			}

			// (Since the attribute catalog will never be empty(why?), we do not
			//  need to handle the case of the linked list becoming empty - i.e
			//  every block of the attribute catalog gets released.)

			// call releaseBlock()
			attrBuffer.releaseBlock( );
		}

		// (the following part is only relevant once indexing has been implemented)
		// if index exists for the attribute (rootBlock != -1), call bplus destroy
		if ( rootBlock != -1 ) {
			// delete the bplus tree rooted at rootBlock using
			// BPlusTree::bPlusDestroy(https://www.youtube.com/watch?v=rKsTbGtygCs)
		}
	}

	/*** Delete the entry corresponding to the relation from relation catalog ***/
	// Fetch the header of Relcat block
	HeadInfo relCatHeader;

	/* Decrement the numEntries in the header of the block corresponding to
	   the relation catalog entry and set it back
	*/
	assert_res( relCatBuffer.getHeader( &relCatHeader ), SUCCESS );
	relCatHeader.numEntries--;
	assert_res( relCatBuffer.setHeader( &relCatHeader ), SUCCESS );

	/* Get the slotmap in relation catalog, update it by marking the slot as
	   free(SLOT_UNOCCUPIED) and set it back. */
	std::unique_ptr<unsigned char[]> slotMapPtr( new unsigned char[ relCatHeader.numSlots ] );
	assert_res( relCatBuffer.getSlotMap( slotMapPtr.get( ) ), SUCCESS );
	*( slotMapPtr.get( ) + recId.slot ) = SLOT_UNOCCUPIED;
	assert_res( relCatBuffer.setSlotMap( slotMapPtr.get( ) ), SUCCESS );

	/*** Updating the Relation Cache Table ***/
	/** Update relation catalog record entry (number of records in relation
	   decreased by 1) **/

	// Get the entry corresponding to relation catalog from the relation
	// cache and update the number of records and set it back
	// (using RelCacheTable::setRelCatEntry() function)
	RelCatEntry relCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( RELCAT_RELID, &relCatEntry ), SUCCESS );
	relCatEntry.numRecs--;
	assert_res( RelCacheTable::setRelCatEntry( RELCAT_RELID, &relCatEntry ), SUCCESS );

	/** Update attribute catalog entry (number of records in attribute catalog
	   by numberOfAttributesDeleted) **/
	// i.e., #Records = #Records - numberOfAttributesDeleted

	// Get the entry corresponding to attribute catalog from the relation
	// cache and update the number of records and set it back
	// (using RelCacheTable::setRelCatEntry() function)
	RelCatEntry attrCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( ATTRCAT_RELID, &attrCatEntry ), SUCCESS );
	relCatEntry.numRecs -= numberOfAttributesDeleted;
	assert_res( RelCacheTable::setRelCatEntry( ATTRCAT_RELID, &attrCatEntry ), SUCCESS );

	return SUCCESS;
}

/*
NOTE: the caller is expected to allocate space for the argument `record` based
the relation. This function will only copy the result of the projection onto the
array pointed to by the argument.
*/
int BlockAccess::project( int relId, Attribute* record ) {
	// get the previous search index of the relation relId from the relation
	// cache (use RelCacheTable::getSearchIndex() function)
	RecId prevRecId;
	assert_res( RelCacheTable::getSearchIndex( relId, &prevRecId ), SUCCESS );

	// declare block and slot which will be used to store the record id of the
	// slot we need to check.
	int block, slot;

	/* if the current search index record is invalid(i.e. = {-1, -1})
	   (this only happens when the caller reset the search index)
	*/
	if ( prevRecId.block == -1 && prevRecId.slot == -1 ) {
		// (new project operation. start from beginning)
		// get the first record block of the relation from the relation cache
		// (use RelCacheTable::getRelCatEntry() function of Cache Layer)
		RelCatEntry relCatEntry;
		assert_res( RelCacheTable::getRelCatEntry( relId, &relCatEntry ), SUCCESS );

		// block = first record block of the relation
		// slot = 0
		block = relCatEntry.firstBlk;
		slot  = 0;
	} else {
		// (a project/search operation is already in progress)

		// block = previous search index's block
		// slot = previous search index's slot + 1
		block = prevRecId.block;
		slot  = prevRecId.slot + 1;
	}

	// The following code finds the next record of the relation
	/* Start from the record id (block, slot) and iterate over the remaining
	   records of the relation */
	while ( block != -1 ) {
		// create a RecBuffer object for block (using appropriate constructor!)
		RecBuffer currBuffer( block );
		HeadInfo currHeader;

		// get header of the block using RecBuffer::getHeader() function
		assert_res( currBuffer.getHeader( &currHeader ), SUCCESS );
		std::unique_ptr<unsigned char[]> currSlotMap( new unsigned char[ currHeader.numSlots ] );
		// get slot map of the block using RecBuffer::getSlotMap() function
		assert_res( currBuffer.getSlotMap( currSlotMap.get( ) ), SUCCESS );

		if ( slot >= currHeader.numSlots /* slot >= the number of slots per block*/ ) {
			// (no more slots in this block)
			// update block = right block of block
			block = currHeader.rblock;
			// update slot = 0
			slot = 0;
			// (NOTE: if this is the last block, rblock would be -1. this would
			//        set block = -1 and fail the loop condition )

		} else if ( currSlotMap.get( )[ slot ] == SLOT_UNOCCUPIED /* slot is free */ ) {
			// (i.e slot-th entry in slotMap contains
			// SLOT_UNOCCUPIED)
			slot++;
			// increment slot
		} else {
			// (the next occupied slot / record has been found)
			break;
		}
	}

	if ( block == -1 ) {
		// (a record was not found. all records exhausted)
		return E_NOTFOUND;
	}

	// declare nextRecId to store the RecId of the record found
	RecId nextRecId{ block, slot };

	// set the search index to nextRecId using RelCacheTable::setSearchIndex
	assert_res( RelCacheTable::setSearchIndex( relId, &nextRecId ), SUCCESS );

	/* Copy the record with record id (nextRecId) to the record buffer (record)
	   For this Instantiate a RecBuffer class object by passing the recId and
	   call the appropriate method to fetch the record
	*/
	RecBuffer resBuffer( block );
	assert_res( resBuffer.getRecord( record, slot ), SUCCESS );

	return SUCCESS;
}

int BlockAccess::search( int relId, Attribute* record, char attrName[ ATTR_SIZE ], Attribute attrVal, int op ) {
	// Declare a variable called recid to store the searched record
	RecId recId;

	/* get the attribute catalog entry from the attribute cache corresponding
	to the relation with Id=relid and with attribute_name=attrName  */
	AttrCatEntry attrCatEntry;
	auto res = AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry );

	// if this call returns an error, return the appropriate error code
	if ( res != SUCCESS )
		return res;

	int rootBlock = attrCatEntry.rootBlock;
	if ( rootBlock == -1 ) {
		// get rootBlock from the attribute catalog entry
		/* if Index does not exist for the attribute (check rootBlock == -1) */

		/* search for the record id (recid) corresponding to the attribute with
		   attribute name attrName, with value attrval and satisfying the
		   condition op using linearSearch()
		*/
		recId = BlockAccess::linearSearch( relId, attrName, attrVal, op );
		/* else */
	} else {
		// (index exists for the attribute)

		/* search for the record id (recid) correspoding to the attribute with
		attribute name attrName and with value attrval and satisfying the
		condition op using BPlusTree::bPlusSearch() */
		recId = BPlusTree::bPlusSearch( relId, attrName, attrVal, op );
	}

	// if there's no record satisfying the given condition (recId = {-1, -1})
	//     return E_NOTFOUND;
	if ( recId == RecId{ -1, -1 } ) {
		return E_NOTFOUND;
	}

	/*
	 * Copy the record with record id (recId) to the record buffer (record).
	 * For this, instantiate a RecBuffer class object by passing the recId and
	 * call the appropriate method to fetch the record
	 */
	RecBuffer recBuffer( recId.block );
	assert_res( recBuffer.getRecord( record, recId.slot ), SUCCESS );

	return SUCCESS;
}

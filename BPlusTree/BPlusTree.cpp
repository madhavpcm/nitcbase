#include "BPlusTree.h"
#include <cstring>
#include <vector>

RecId BPlusTree::bPlusSearch( int relId, char attrName[ ATTR_SIZE ], Attribute attrVal, int op ) {
	// declare searchIndex which will be used to store search index for attrName.
	IndexId searchIndex;

	/* get the search index corresponding to attribute with name attrName
	   using AttrCacheTable::getSearchIndex(). */
	assert_res( AttrCacheTable::getSearchIndex( relId, attrName, &searchIndex ), SUCCESS );

	AttrCatEntry attrCatEntry;
	/* load the attribute cache entry into attrCatEntry using
	 AttrCacheTable::getAttrCatEntry(). */
	assert_res( AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

	// declare variables block and index which will be used during search
	int block, index;

	if ( searchIndex == IndexId{ -1, -1 } /* searchIndex == {-1, -1}*/ ) {
		// (search is done for the first time)

		// start the search from the first entry of root.
		block = attrCatEntry.rootBlock;
		index = 0;

		if ( block == -1 /* attrName doesn't have a B+ tree (block == -1)*/ ) {
			return RecId{ -1, -1 };
		}

	} else {
		/*a valid searchIndex points to an entry in the leaf index of the
		attribute's B+ Tree which had previously satisfied the op for the given
		attrVal.*/

		block = searchIndex.block;
		index = searchIndex.index + 1; // search is resumed from the next index.

		// load block into leaf using IndLeaf::IndLeaf().
		IndLeaf leaf( block );

		// declare leafHead which will be used to hold the header of leaf.
		HeadInfo leafHead;

		// load header into leafHead using BlockBuffer::getHeader().
		assert_res( leaf.getHeader( &leafHead ), SUCCESS );

		if ( index >= leafHead.numEntries ) {
			/* (all the entries in the block has been searched; search from the
			beginning of the next leaf index block. */

			// update block to rblock of current block and index to 0.
			block = leafHead.rblock;
			index = 0;

			if ( block == -1 ) {
				// (end of linked list reached - the search is done.)
				return RecId{ -1, -1 };
			}
		}
	}

	/******  Traverse through all the internal nodes according to value
  *of attrVal
	   and the operator op ******/

	/* (This section is only needed when
	 *- search
	 * restarts from the root block (when searchIndex is reset by caller)
	 *  																															-
	 *root is not a leaf If there was a valid search index, then we are already at
	 *a leaf block and the test condition in the following loop will fail)
	 */

	auto blockType = StaticBuffer::getStaticBlockType( block );
	while ( blockType == IND_INTERNAL ) {

		// load the block into internalBlk using IndInternal::IndInternal().
		IndInternal internalBlk( block );

		HeadInfo intHead;

		// load the header of internalBlk into intHead using
		// BlockBuffer::getHeader()
		assert_res( internalBlk.getHeader( &intHead ), SUCCESS );

		// declare intEntry which will be used to store an entry of internalBlk.
		InternalEntry intEntry;

		if ( op == NE || op == LT || op == LE /* op is one of NE, LT, LE */ ) {
			/*
			- NE: need to search the entire linked list of leaf indices of the B+
			Tree, starting from the leftmost leaf index. Thus, always move to the
			left.

			- LT and LE: the attribute values are arranged in ascending order in the
			leaf indices of the B+ Tree. Values that satisfy these conditions, if
			any exist, will always be found in the left-most leaf index. Thus,
			always move to the left.
			*/

			// load entry in the first slot of the block into intEntry
			// using IndInternal::getEntry().
			assert_res( internalBlk.getEntry( &intEntry, 0 ), SUCCESS );

			block = intEntry.lChild;

		} else {
			/*
			- EQ, GT and GE: move to the left child of the first entry that is
			greater than (or equal to) attrVal
			(we are trying to find the first entry that satisfies the condition.
			since the values are in ascending order we move to the left child which
			might contain more entries that satisfy the condition)
			*/

			/*
			 traverse through all entries of internalBlk and find an entry that
			 satisfies the condition.
			 if op == EQ or GE, then intEntry.attrVal >= attrVal
			 if op == GT, then intEntry.attrVal > attrVal
			 Hint: the helper function compareAttrs() can be used for comparing
			*/
			int i = 0;
			for ( i = 0; i < intHead.numEntries; i++ ) {
				assert_res( internalBlk.getEntry( &intEntry, i ), SUCCESS );
				int cmpVal = compareAttrs( &intEntry.attrVal, &attrVal, attrCatEntry.attrType );
				if ( ( ( op == EQ || op == GE ) && cmpVal >= 0 ) || ( op == GT && cmpVal > 0 ) ) {
					break;
				}
			}
			std::cout << '\n';

			if ( i < intHead.numEntries /* such an entry is found*/ ) {
				// move to the left child of that entry
				block = intEntry.lChild; // left child of the entry

			} else {
				// move to the right child of the last entry of the block
				// i.e numEntries - 1 th entry of the block
				assert_res( internalBlk.getEntry( &intEntry, intHead.numEntries - 1 ), SUCCESS );

				block = intEntry.rChild; // right child of last entry
			}
		}
		blockType = StaticBuffer::getStaticBlockType( block );
	}

	// NOTE: `block` now has the block number of a leaf index block.

	/******  Identify the first leaf index entry from the current position
	 *that satisfies our
	   condition (moving right) ******/

	while ( block != -1 ) {
		// load the block into leafBlk using IndLeaf::IndLeaf().
		IndLeaf leafBlk( block );
		HeadInfo leafHead;

		// load the header to leafHead using BlockBuffer::getHeader().
		assert_res( leafBlk.getHeader( &leafHead ), SUCCESS );

		// declare leafEntry which will be used to store an entry from leafBlk
		Index leafEntry;

		while ( index < leafHead.numEntries /*index < numEntries in leafBlk*/ ) {

			// load entry corresponding to block and index into leafEntry
			// using IndLeaf::getEntry().
			assert_res( leafBlk.getEntry( ( void* )&leafEntry, index ), SUCCESS );

			/**
			 * comparison between
			 * leafEntry's attribute
			 * value and input
			 * attrVal using
			 * compareAttrs()
			 **/
			int cmpVal = compareAttrs( &leafEntry.attrVal, &attrVal, attrCatEntry.attrType );

			if ( ( op == EQ && cmpVal == 0 ) || ( op == LE && cmpVal <= 0 ) || ( op == LT && cmpVal < 0 ) ||
				 ( op == GT && cmpVal > 0 ) || ( op == GE && cmpVal >= 0 ) || ( op == NE && cmpVal != 0 ) ) {
				// (entry satisfying the condition found)

				// set search index to {block, index}
				searchIndex = { block, index };
				assert_res( AttrCacheTable::setSearchIndex( relId, attrName, &searchIndex ), SUCCESS );
				// return the recId {leafEntry.block, leafEntry.slot}.
				return { leafEntry.block, leafEntry.slot };

			} else if ( ( op == EQ || op == LE || op == LT ) && cmpVal > 0 ) {
				/*future entries will not satisfy EQ, LE, LT since the values
				  are
				   arranged in ascending order in the leaves */

				return { -1, -1 };
				// return RecId {-1, -1};
			}

			// search next index.
			++index;
		}

		/*only for NE operation do we have to check the entire linked list;
		for all the other op it is guaranteed that the block being searched
		will have an entry, if it exists, satisying that op. */
		if ( op != NE ) {
			break;
		}
		// block = next block in the linked list, i.e., the rblock in leafHead.
		// update index to 0.

		block = leafHead.rblock;
		index = 0;
	}

	return { -1, -1 };
	// no entry satisying the op was found; return the recId {-1,-1}
}

int BPlusTree::bPlusCreate( int relId, char attrName[ ATTR_SIZE ] ) {

	// if relId is either RELCAT_RELID or ATTRCAT_RELID:
	//     return E_NOTPERMITTED;
	if ( relId == RELCAT_RELID || relId == ATTRCAT_RELID )
		return E_NOTPERMITTED;

	// get the attribute catalog entry of attribute `attrName`
	// using AttrCacheTable::getAttrCatEntry()
	AttrCatEntry attrCatEntry;
	auto res = AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry );

	if ( res != SUCCESS )
		return res;
	// if getAttrCatEntry fails
	//     return the error code from getAttrCatEntry

	if ( attrCatEntry.rootBlock != -1 /* an index already exists for the attribute (check rootBlock field) */ ) {
		return SUCCESS;
	}

	/******Creating a new B+ Tree ******/

	// get a free leaf block using constructor 1 to allocate a new block
	IndLeaf rootBlockBuf;

	// (if the block could not be allocated, the appropriate error code
	//  will be stored in the blockNum member field of the object)

	// declare rootBlock to store the blockNumber of the new leaf block
	int rootBlock = rootBlockBuf.getBlockNum( );

	// if there is no more disk space for creating an index
	if ( rootBlock < 0 ) {
		return rootBlock;
	}

	attrCatEntry.rootBlock = rootBlock;
	assert_res( AttrCacheTable::setAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

	// load the relation catalog entry into relCatEntry
	// using RelCacheTable::getRelCatEntry().

	RelCatEntry relCatEntry;
	assert_res( RelCacheTable::getRelCatEntry( relId, &relCatEntry ), SUCCESS );

	int block = relCatEntry.firstBlk /* first record block of the relation */;

	/***** Traverse all the blocks in the relation and insert them one
	   the B+ Tree *****/
	while ( block != -1 ) {

		// declare a RecBuffer object for `block` (using appropriate constructor)
		RecBuffer currBuffer( block );

		std::unique_ptr<unsigned char[]> currSlotMap( new unsigned char[ relCatEntry.numSlotsPerBlk ] );

		// load the slot map into slotMap using RecBuffer::getSlotMap().
		assert_res(currBuffer.getSlotMap( currSlotMap.get( ) ), SUCCESS);

		// for every occupied slot of the block
		for ( int i = 0; i < relCatEntry.numSlotsPerBlk; i++ ) {
			if ( currSlotMap.get( )[ i ] == SLOT_OCCUPIED ) {
				std::unique_ptr<union Attribute[]> record( new union Attribute[ relCatEntry.numAttrs ] );
				// load the record corresponding to the slot into `record`
				// using RecBuffer::getRecord().
				currBuffer.getRecord( record.get( ), i );

				// declare recId and store the rec-id of this record in it
				// RecId recId{block, slot};
				RecId recId{ block, i };

				// insert the attribute value corresponding to attrName from the record
				// into the B+ tree using bPlusInsert.
				// (note that bPlusInsert will destroy any existing bplus tree if
				// insert fails i.e when disk is full)
				// retVal = bPlusInsert(relId, attrName, attribute value, recId);
				auto retVal = bPlusInsert( relId, attrName, record[ attrCatEntry.offset ], recId );

				// if (retVal == E_DISKFULL) {
				//     // (unable to get enough blocks to build the B+ Tree.)
				//     return E_DISKFULL;
				// }
				if ( retVal == E_DISKFULL ) {
					return E_DISKFULL;
				}
			}
		}

		// get the header of the block using BlockBuffer::getHeader()
		HeadInfo currHeader;
		currBuffer.getHeader( &currHeader );
		block = currHeader.rblock;

		// set block = rblock of current block (from the header)
	}

	return SUCCESS;
}

int BPlusTree::bPlusDestroy( int rootBlockNum ) {
	if ( rootBlockNum < 0 ||
		 rootBlockNum >= DISK_BLOCKS /*rootBlockNum lies outside the valid range [0,DISK_BLOCKS-1]*/ ) {
		return E_OUTOFBOUND;
	}

	int type = StaticBuffer::getStaticBlockType( rootBlockNum );

	if ( type == IND_LEAF ) {
		// declare an instance of IndLeaf for rootBlockNum using appropriate
		// constructor
		IndLeaf leaf( rootBlockNum );
		// release the block using BlockBuffer::releaseBlock().
		leaf.releaseBlock( );
		return SUCCESS;

	} else if ( type == IND_INTERNAL ) {
		// declare an instance of IndInternal for rootBlockNum using appropriate
		// constructor
		IndInternal internal( rootBlockNum );
		HeadInfo internalHeader;

		// load the header of the block using BlockBuffer::getHeader().
		internal.getHeader( &internalHeader );

		/*iterate through all the entries of the internalBlk and destroy the lChild
		of the first entry and rChild of all entries using
		BPlusTree::bPlusDestroy(). (the rchild of an entry is the same as the lchild
		of the next entry. take care not to delete overlapping children more than
		once ) */

		InternalEntry entry;
		for ( int i = 0; i < internalHeader.numEntries; i++ ) {
			internal.getEntry( &entry, i );
			bPlusDestroy( entry.lChild );
		}
		internal.getEntry( &entry, internalHeader.numEntries - 1 );
		bPlusDestroy( entry.rChild );

		// release the block using BlockBuffer::releaseBlock().
		internal.releaseBlock( );

		return SUCCESS;

	} else {
		// (block is not an index block.)
		return E_INVALIDBLOCK;
	}
}

int BPlusTree::bPlusInsert( int relId, char attrName[ ATTR_SIZE ], Attribute attrVal, RecId recId ) {
	// get the attribute cache entry corresponding to attrName
	// using AttrCacheTable::getAttrCatEntry().
	AttrCatEntry attrCatEntry;
	auto res = AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry );

	// if getAttrCatEntry() failed
	//     return the error code
	if ( res != SUCCESS )
		return res;

	int rootBlock = attrCatEntry.rootBlock; /* rootBlock of B+ Tree (from attrCatEntry) */
	;

	if ( rootBlock == -1 /* there is no index on attribute (rootBlock is -1) */ ) {
		return E_NOINDEX;
	}

	// find the leaf block to which insertion is to be done using the
	// findLeafToInsert() function

	/* findLeafToInsert(root block num, attrVal, attribute type) */;

	int leafBlkNum = findLeafToInsert( rootBlock, attrVal, attrCatEntry.attrType );

	Index indVal;
	indVal.attrVal = attrVal;
	indVal.block   = recId.block;
	indVal.slot	   = recId.slot;
	// insert the attrVal and recId to the leaf block at blockNum using the
	// insertIntoLeaf() function.
	// declare a struct Index with attrVal = attrVal, block = recId.block and
	// slot = recId.slot to pass as argument to the function.
	// insertIntoLeaf(relId, attrName, leafBlkNum, Index entry)
	res = insertIntoLeaf( relId, attrName, leafBlkNum, indVal );
	// NOTE: the insertIntoLeaf() function will propagate the insertion to the
	//       required internal nodes by calling the required helper functions
	//       like insertIntoInternal() or createNewRoot()

	if ( res == E_DISKFULL /*insertIntoLeaf() returns E_DISKFULL */ ) {
		// destroy the existing B+ tree by passing the rootBlock to bPlusDestroy().
		assert_res( BPlusTree::bPlusDestroy( rootBlock ), SUCCESS );

		// update the rootBlock of attribute catalog cache entry to -1 using
		// AttrCacheTable::setAttrCatEntry().
		attrCatEntry.rootBlock = -1;
		assert_res( AttrCacheTable::setAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

		return E_DISKFULL;
	}

	return SUCCESS;
}

int BPlusTree::findLeafToInsert( int rootBlock, Attribute attrVal, int attrType ) {
	int blockNum = rootBlock;

	int blockType = StaticBuffer::getStaticBlockType( blockNum );
	while ( blockType != IND_LEAF /*block is not of type IND_LEAF */ ) { // use

		// declare an IndInternal object for block using appropriate constructor
		IndInternal intBuffer( blockNum );
		HeadInfo intHeader;

		// get header of the block using BlockBuffer::getHeader()
		assert_res( intBuffer.getHeader( &intHeader ), SUCCESS );

		/* iterate through all the entries, to find the first entry whose
		InternalEntry entry; */
		InternalEntry intEntry;
		int i;
		for ( i = 0; i < intHeader.numEntries; i++ ) {
			assert_res( intBuffer.getEntry( &intEntry, i ), SUCCESS );
			if ( compareAttrs( &intEntry.attrVal, &attrVal, attrType ) > 0 ) {
				break;
			}
		}

		if ( i == intHeader.numEntries /*no such entry is found*/ ) {
			// set blockNum = rChild of (nEntries-1)'th entry of the block
			// (i.e. rightmost child of the block)
			blockNum = intEntry.rChild;

		} else {
			// set blockNum = lChild of the entry that was found
			blockNum = intEntry.lChild;
		}
		blockType = StaticBuffer::getStaticBlockType( blockNum );
	}

	return blockNum;
}

int BPlusTree::insertIntoLeaf( int relId, char attrName[ ATTR_SIZE ], int blockNum, Index indexEntry ) {
	// get the attribute cache entry corresponding to attrName
	// using AttrCacheTable::getAttrCatEntry().
	AttrCatEntry attrCatEntry;
	assert_res( AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

	// declare an IndLeaf instance for the block using appropriate constructor
	IndLeaf leafBuffer( blockNum );
	HeadInfo leafHeader;

	// store the header of the leaf index block into blockHeader
	// using BlockBuffer::getHeader()
	assert_res( leafBuffer.getHeader( &leafHeader ), SUCCESS );

	// the following variable will be used to store a list of index entries with
	// existing indices + the new index to insert
	std::vector<Index> indices( leafHeader.numEntries );

	/*
	Iterate through all the entries in the block and copy them to the array
	indices. Also insert `indexEntry` at appropriate position in the indices array
	maintaining the ascending order.
	- use IndLeaf::getEntry() to get the entry
	- use compareAttrs() declared in BlockBuffer.h to compare two Attribute
	structs
	*/
	int insertIndex = -1;
	bool got		= false;

	for ( int i = 0; i < leafHeader.numEntries; i++ ) {
		assert_res( leafBuffer.getEntry( &indices[ i ], i ), SUCCESS );
		if ( !got && compareAttrs( &indices[ i ].attrVal, &indexEntry.attrVal, attrCatEntry.attrType ) >= 0 ) {
			insertIndex = i;
			got			= true;
		}
	}
	// leafblock is empty
	if ( !got ) {
		indices.push_back( indexEntry );
	} else {
		indices.insert( indices.begin( ) + insertIndex, indexEntry );
	}

	if ( leafHeader.numEntries != MAX_KEYS_LEAF ) {
		// (leaf block has not reached max limit)

		// increment blockHeader.numEntries and update the header of block
		// using BlockBuffer::setHeader().
		leafHeader.numEntries++;
		assert_res( leafBuffer.setHeader( &leafHeader ), SUCCESS );

		// iterate through all the entries of the array `indices` and populate the
		// entries of block with them using IndLeaf::setEntry().
		for ( int i = 0; i < leafHeader.numEntries; i++ ) {
			leafBuffer.setEntry( &indices[ i ], i );
		}

		return SUCCESS;
	}

	// If we reached here, the `indices` array has more than entries than can fit
	// in a single leaf index block. Therefore, we will need to split the entries
	// in `indices` between two leaf blocks. We do this using the splitLeaf()
	// function. This function will return the blockNum of the newly allocated
	// block or E_DISKFULL if there are no more blocks to be allocated.

	int newRightBlk = splitLeaf( blockNum, indices.data( ) );

	// if splitLeaf() returned E_DISKFULL
	//     return E_DISKFULL
	if ( newRightBlk == E_DISKFULL )
		return E_DISKFULL;

	int res = SUCCESS;
	if ( leafHeader.pblock != -1 /* the current leaf block was not the root */ ) {
		// check pblock in header
		// insert the middle value from `indices` into the parent block using the
		// insertIntoInternal() function. (i.e the last value of the left block)
		InternalEntry mid;
		mid.lChild	= blockNum;
		mid.rChild	= newRightBlk;
		mid.attrVal = indices[ MIDDLE_INDEX_LEAF ].attrVal;

		res = insertIntoInternal( relId, attrName, leafHeader.pblock, mid );

	} else {
		// the current block was the root block and is now split. a new internal
		// index block needs to be allocated and made the root of the tree. To do
		// this, call the createNewRoot() function with the following arguments

		// createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal,
		//               current block, new right block)
		res = createNewRoot( relId, attrName, indices[ MIDDLE_INDEX_LEAF ].attrVal, blockNum, newRightBlk );
	}

	return res;
}

int BPlusTree::splitLeaf( int leafBlockNum, Index indices[] ) {
	// declare rightBlk, an instance of IndLeaf using constructor 1 to obtain new
	// leaf index block that will be used as the right block in the splitting
	IndLeaf rightBlk;

	// declare leftBlk, an instance of IndLeaf using constructor 2 to read from
	// the existing leaf block
	IndLeaf leftBlk( leafBlockNum );

	int rightBlkNum = rightBlk.getBlockNum( ) /* block num of right blk */;
	int leftBlkNum	= leafBlockNum /* block num of left blk */;

	if ( rightBlkNum == E_DISKFULL /* newly allocated block has blockNum E_DISKFULL */ ) {
		//(failed to obtain a new leaf index block because the disk is full)
		return E_DISKFULL;
	}

	HeadInfo leftBlkHeader, rightBlkHeader;
	// get the headers of left block and right block using
	// BlockBuffer::getHeader()
	assert_res( leftBlk.getHeader( &leftBlkHeader ), SUCCESS );
	assert_res( rightBlk.getHeader( &rightBlkHeader ), SUCCESS );

	// set rightBlkHeader with the following values
	// - number of entries = (MAX_KEYS_LEAF+1)/2 = 32,
	rightBlkHeader.numEntries = ( MAX_KEYS_LEAF + 1 ) / 2;
	// - pblock = pblock of leftBlk
	rightBlkHeader.pblock = leftBlkHeader.pblock;
	// - lblock = leftBlkNum
	rightBlkHeader.lblock = leftBlkNum;
	// - rblock = rblock of leftBlk
	rightBlkHeader.rblock = leftBlkHeader.rblock;
	// and update the header of rightBlk using BlockBuffer::setHeader()
	assert_res( rightBlk.setHeader( &rightBlkHeader ), SUCCESS );

	// set leftBlkHeader with the following values
	// - number of entries = (MAX_KEYS_LEAF+1)/2 = 32
	leftBlkHeader.numEntries = ( MAX_KEYS_LEAF + 1 ) / 2;
	// - rblock = rightBlkNum
	leftBlkHeader.rblock = rightBlkNum;
	// and update the header of leftBlk using BlockBuffer::setHeader() */
	assert_res( leftBlk.setHeader( &leftBlkHeader ), SUCCESS );
	// set the first 32 entries of leftBlk = the first 32 entries of indices array
	for ( int i = 0; i < leftBlkHeader.numEntries; i++ ) {
		assert_res( leftBlk.setEntry( &indices[ i ], i ), SUCCESS );
		assert_res( rightBlk.setEntry( &indices[ i + MIDDLE_INDEX_LEAF + 1 ], i ), SUCCESS );
	}
	// and set the first 32 entries of newRightBlk = the next 32 entries of
	// indices array using IndLeaf::setEntry().

	return rightBlkNum;
}

int BPlusTree::insertIntoInternal(
	int relId, char attrName[ ATTR_SIZE ], int intBlockNum, InternalEntry intIndexEntry ) {
	// get the attribute cache entry corresponding to attrName
	// using AttrCacheTable::getAttrCatEntry().
	AttrCatEntry attrCatEntry;
	assert_res( AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

	// declare intBlk, an instance of IndInternal using constructor 2 for the
	// block corresponding to intBlockNum
	IndInternal intBlk( intBlockNum );
	HeadInfo intBlkHeader;
	// load blockHeader with header of intBlk using BlockBuffer::getHeader().
	assert_res( intBlk.getHeader( &intBlkHeader ), SUCCESS );

	// declare internalEntries to store all existing entries + the new entry
	std::vector<InternalEntry> internalEntries( intBlkHeader.numEntries );

	/*
	Iterate through all the entries in the block and copy them to the array
	`internalEntries`. Insert `indexEntry` at appropriate position in the
	array maintaining the ascending order.
	* - use IndInternal::getEntry() to get the entry
	* - use compareAttrs() to compare two structs of
	type Attribute

	Update the lChild of the internalEntry immediately following the newly added
	entry to the rChild of the newly added entry.
	*/
	int insertIndex = -1;
	bool got		= false;

	for ( int i = 0; i < intBlkHeader.numEntries; i++ ) {
		assert_res( intBlk.getEntry( &internalEntries[ i ], i ), SUCCESS );
		if ( !got &&
			 compareAttrs( &internalEntries[ i ].attrVal, &intIndexEntry.attrVal, attrCatEntry.attrType ) >= 0 ) {
			insertIndex = i;
			got			= true;
		}
	}
	// leafblock is empty
	if ( insertIndex == -1 ) {
		internalEntries.push_back( intIndexEntry );
	} else {
		internalEntries.insert( internalEntries.begin( ) + insertIndex, intIndexEntry );
	}

	if ( intBlkHeader.numEntries != MAX_KEYS_INTERNAL ) {
		// (internal index block has not reached max limit)

		// increment blockheader.numEntries and update the header of intBlk
		// using BlockBuffer::setHeader().
		intBlkHeader.numEntries++;
		assert_res( intBlk.setHeader( &intBlkHeader ), SUCCESS );

		// iterate through all entries in internalEntries array and populate the
		// entries of intBlk with them using IndInternal::setEntry().
		for ( int i = 0; i < intBlkHeader.numEntries; i++ ) {
			assert_res( intBlk.setEntry( &internalEntries[ i ], i ), SUCCESS );
		}

		return SUCCESS;
	}

	// If we reached here, the `internalEntries` array has more than entries than
	// can fit in a single internal index block. Therefore, we will need to split
	// the entries in `internalEntries` between two internal index blocks. We do
	// this using the splitInternal() function.
	// This function will return the blockNum of the newly allocated block or
	// E_DISKFULL if there are no more blocks to be allocated.

	int newRightBlk = splitInternal( intBlockNum, internalEntries.data( ) );

	if ( newRightBlk == E_DISKFULL /* splitInternal() returned E_DISKFULL */ ) {

		// Using bPlusDestroy(), destroy the right subtree, rooted at
		// intEntry.rChild. This corresponds to the tree built up till now that has
		// not yet been connected to the existing B+ Tree
		bPlusDestroy( intIndexEntry.rChild );

		return E_DISKFULL;
	}

	int res = SUCCESS;
	if ( intBlkHeader.pblock != -1 /* the current block was not the root */ ) { // (check pblock in header)
		// insert the middle value from `internalEntries` into the parent block
		// using the insertIntoInternal() function (recursively).
		// the middle value will be at index 50 (given by constant
		// MIDDLE_INDEX_INTERNAL)
		InternalEntry a;

		// create a struct InternalEntry with lChild = current block, rChild =
		a.lChild  = intBlockNum;
		a.rChild  = newRightBlk;
		a.attrVal = internalEntries[ MIDDLE_INDEX_INTERNAL ].attrVal;
		// newRightBlk and attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal
		// and pass it as argument to the insertIntoInternalFunction as follows

		// insertIntoInternal(relId, attrName, parent of current block, new internal
		// entry)
		res = BPlusTree::insertIntoInternal( relId, attrName, intBlkHeader.pblock, a );
	} else {
		// the current block was the root block and is now split. a new internal
		// index block needs to be allocated and made the root of the tree. To do
		// this, call the createNewRoot() function with the following arguments

		// createNewRoot(relId, attrName,
		//               internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,
		//               current block, new right block)
		res = BPlusTree::createNewRoot(
			relId, attrName, internalEntries[ MIDDLE_INDEX_INTERNAL ].attrVal, intBlockNum, newRightBlk );
	}

	return res;
}

int BPlusTree::splitInternal( int intBlockNum, InternalEntry internalEntries[] ) {
	// declare rightBlk, an instance of IndInternal using constructor 1 to obtain
	// new internal index block that will be used as the right block in the
	// splitting
	IndInternal rightBlk;
	IndInternal leftBlk( intBlockNum );

	// declare leftBlk, an instance of IndInternal using constructor 2 to read
	// from the existing internal index block

	int rightBlkNum = rightBlk.getBlockNum( ) /* block num of right blk */;
	int leftBlkNum	= intBlockNum /* block num of left blk */;

	if ( rightBlkNum == E_DISKFULL /* newly allocated block has blockNum E_DISKFULL */ ) {
		//(failed to obtain a new internal index block because the disk is full)
		return E_DISKFULL;
	}

	HeadInfo leftBlkHeader, rightBlkHeader;
	// get the headers of left block and right block using
	// BlockBuffer::getHeader()
	assert_res(leftBlk.getHeader( &leftBlkHeader ), SUCCESS);
	assert_res(rightBlk.getHeader( &rightBlkHeader ), SUCCESS);

	// set rightBlkHeader with the following values
	// - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
	rightBlkHeader.numEntries = ( MAX_KEYS_INTERNAL ) / 2;
	// - pblock = pblock of leftBlk
	rightBlkHeader.pblock = leftBlkHeader.pblock;
	// and update the header of rightBlk using BlockBuffer::setHeader()
	assert_res(rightBlk.setHeader( &rightBlkHeader ), SUCCESS);

	// set leftBlkHeader with the following values
	// - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
	leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
	// - rblock = rightBlkNum
	leftBlkHeader.rblock = rightBlkNum;
	// and update the header using BlockBuffer::setHeader()
	assert_res(leftBlk.setHeader( &leftBlkHeader ), SUCCESS);

	/*
	- set the first 50 entries of leftBlk = index 0 to 49 of internalEntries
	  array
	- set the first 50 entries of newRightBlk = entries from index 51 to 100
	  of internalEntries array using IndInternal::setEntry().
	  (index 50 will be moving to the parent internal index block)
	*/
	for ( int i = 0; i < MAX_KEYS_INTERNAL / 2; i++ ) {
		leftBlk.setEntry( &internalEntries[ i ], i );
		rightBlk.setEntry( &internalEntries[ i + MAX_KEYS_INTERNAL / 2 + 1 ], i );
	}

	int type = StaticBuffer::getStaticBlockType( internalEntries[ 0 ].rChild );
	/* block type of a child of any entry of the internalEntries array */;
	//            (use StaticBuffer::getStaticBlockType())

	for ( int i = 0; i < MAX_KEYS_INTERNAL / 2; i++ /* each child block of the new right block */ ) {
		// declare an instance of BlockBuffer to access the child block using
		// constructor 2
		BlockBuffer childBuffer( internalEntries[ i + ( MAX_KEYS_INTERNAL / 2 ) ].lChild );
		HeadInfo childHeader;
		assert_res(childBuffer.getHeader( &childHeader ), SUCCESS);

		childHeader.pblock = rightBlkNum;
		// update pblock of the block to rightBlkNum using BlockBuffer::getHeader()
		// and BlockBuffer::setHeader().
		assert_res(childBuffer.setHeader( &childHeader ), SUCCESS);
	}

	return rightBlkNum;
}

int BPlusTree::createNewRoot( int relId, char attrName[ ATTR_SIZE ], Attribute attrVal, int lChild, int rChild ) {
	// get the attribute cache entry corresponding to attrName
	// using AttrCacheTable::getAttrCatEntry().
	AttrCatEntry attrCatEntry;
	assert_res( AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry ), SUCCESS );

	// declare newRootBlk, an instance of IndInternal using appropriate
	// constructor to allocate a new internal index block on the disk
	IndInternal newRootBlk;

	int newRootBlkNum = newRootBlk.getBlockNum( ) /* block number of newRootBlk */;

	if ( newRootBlkNum == E_DISKFULL ) {
		// (failed to obtain an empty internal index block because the disk is full)

		// Using bPlusDestroy(), destroy the right subtree, rooted at rChild.
		// This corresponds to the tree built up till now that has not yet been
		// connected to the existing B+ Tree
		assert_res(BPlusTree::bPlusDestroy( rChild ), SUCCESS);

		return E_DISKFULL;
	}

	HeadInfo rootBlkHeader;
	assert_res( newRootBlk.getHeader( &rootBlkHeader ), SUCCESS );
	// update the header of the new block with numEntries = 1 using
	// BlockBuffer::getHeader() and BlockBuffer::setHeader()
	rootBlkHeader.numEntries = 1;
	assert_res( newRootBlk.setHeader( &rootBlkHeader ), SUCCESS );

	// create a struct InternalEntry with lChild, attrVal and rChild from the
	InternalEntry intEntry;
	intEntry.lChild	 = lChild;
	intEntry.rChild	 = rChild;
	intEntry.attrVal = attrVal;
	// arguments and set it as the first entry in newRootBlk using
	// IndInternal::setEntry()
	newRootBlk.setEntry( &intEntry, 0 );
	// declare BlockBuffer instances for the `lChild` and `rChild` blocks using
	BlockBuffer lBlock( lChild );
	BlockBuffer rBlock( rChild );
	HeadInfo lHeader, rHeader;
	assert_res( lBlock.getHeader( &lHeader ), SUCCESS );
	assert_res( rBlock.getHeader( &rHeader ), SUCCESS );
	// appropriate constructor and update the pblock of those blocks to
	// `newRootBlkNum` using BlockBuffer::getHeader() and BlockBuffer::setHeader()
	lHeader.pblock = rHeader.pblock = newRootBlkNum;
	assert_res( lBlock.setHeader( &lHeader ), SUCCESS );
	assert_res( rBlock.setHeader( &rHeader ), SUCCESS );

	attrCatEntry.rootBlock = newRootBlkNum;
	AttrCacheTable::setAttrCatEntry( relId, attrName, &attrCatEntry );
	// update rootBlock = newRootBlkNum for the entry corresponding to `attrName`
	// in the attribute cache using AttrCacheTable::setAttrCatEntry().

	return SUCCESS;
}

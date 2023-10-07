#include "BlockBuffer.h"
#include "../define/constants.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>

int BlockBuffer::getHeader( struct HeadInfo* head ) {
	unsigned char* buffer;
	int ret = this->loadBlockAndGetBufferPtr( &buffer );
	if ( ret != SUCCESS )
		return ret;

	std::copy( buffer, buffer + sizeof( HeadInfo ), reinterpret_cast<unsigned char*>( head ) );

	return SUCCESS;
}

/*
 * @param rec
 * @param slotNum
 */
int RecBuffer::getRecord( union Attribute* rec, int slotNum ) {
	HeadInfo head;
	this->getHeader( &head );
	int attrCount = head.numAttrs;
	int slotCount = head.numSlots;

	// read the block at this.blockNum into a buffer
	unsigned char* buffer;
	int ret = this->loadBlockAndGetBufferPtr( &buffer );
	if ( ret != SUCCESS )
		return ret;

	/* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize
	   * slotNum)
	   - each record will have size attrCount * ATTR_SIZE
	   - slotMap will be of size slotCount
	   */
	int recordSize			   = attrCount * ATTR_SIZE;
	unsigned char* slotPointer = buffer + ( size_t )HEADER_SIZE + SLOTMAPSIZE( attrCount ) + recordSize * slotNum;

	// load the record into the rec data structure
	std::copy( slotPointer, slotPointer + recordSize, reinterpret_cast<unsigned char*>( rec ) );
	return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr( unsigned char** buffer ) {
	if(blockNum < 0 || blockNum >= DISK_BLOCKS) {
		std::cout << "Invalid Block Number" ;
		exit(-1);
	}
		
	int bufferNum = StaticBuffer::getBufferNum( this->blockNum );

	if ( bufferNum != E_BLOCKNOTINBUFFER ) {
		for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
			if ( StaticBuffer::metainfo[ bufferIndex ].free == false ) {
				StaticBuffer::metainfo[ bufferIndex ].timeStamp++;
			}
		}
		StaticBuffer::metainfo[ bufferNum ].timeStamp = 0;
	} else {
		int freeBuffer = StaticBuffer::getFreeBuffer( this->blockNum );

		if ( freeBuffer == E_OUTOFBOUND )
			return E_OUTOFBOUND;

		assert_res( Disk::readBlock( StaticBuffer::blocks[ freeBuffer ].data( ), this->blockNum ), SUCCESS );
		bufferNum = freeBuffer;
	}
	*buffer = StaticBuffer::blocks[ bufferNum ].data( );

	// store the pointer to this buffer (blocks[bufferNum]) in *buffPtr

	return SUCCESS;
}

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap( unsigned char* slotMap ) {
	unsigned char* bufferPtr;

	// get the starting address of the buffer containing the block using
	// loadBlockAndGetBufferPtr().
	int ret = loadBlockAndGetBufferPtr( &bufferPtr );
	if ( ret != SUCCESS ) {
		return ret;
	}

	struct HeadInfo header;
	// get the header of the block using getHeader() function
	assert_res( this->getHeader( &header ), SUCCESS );

	int slotCount = header.numSlots;

	// get a pointer to the beginning of the slotmap in memory by offsetting
	// HEADER_SIZE
	unsigned char* slotMapInBuffer = bufferPtr + HEADER_SIZE;

	// copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
	std::copy( slotMapInBuffer, slotMapInBuffer + slotCount, slotMap );
	return SUCCESS;
}

int compareAttrs( Attribute* attr1, Attribute* attr2, int attrType ) {
	double diff;
	if ( attrType == STRING ) {
		diff = std::strcmp( attr1->sVal, attr2->sVal );
	} else {
		diff = attr1->nVal - attr2->nVal;
	}

	if ( diff > 0 )
		return 1;
	if ( diff < 0 )
		return -1;
	return 0;
}

int RecBuffer::setRecord( union Attribute* rec, int slotNum ) {
	uint8_t* bufferPtr;
	/* get the starting address of the buffer containing the block
	   using loadBlockAndGetBufferPtr(&bufferPtr). */
	auto res = loadBlockAndGetBufferPtr( &bufferPtr );
	if ( res != SUCCESS )
		return res;

	HeadInfo headerInfo;
	this->getHeader( &headerInfo );

	size_t numAttributes = headerInfo.numAttrs;
	size_t numSlots		 = headerInfo.numSlots;

	if ( slotNum < 0 || slotNum >= numSlots )
		return E_OUTOFBOUND;

	uint8_t* offsetPtr =
		bufferPtr + ( size_t )HEADER_SIZE + numSlots + slotNum * ( numAttributes * sizeof( union Attribute ) );
	// std::cout << std::copy( reinterpret_cast<uint8_t*>( rec ),
	//	reinterpret_cast<uint8_t*>( rec ) + numAttributes * sizeof( union
	// Attribute ), offsetPtr ) - offsetPtr << '\n';
	std::memcpy( offsetPtr, rec, numAttributes * ATTR_SIZE );

	StaticBuffer::setDirtyBit( this->blockNum );

	return SUCCESS;
}

int BlockBuffer::setHeader( struct HeadInfo* head ) {

	unsigned char* bufferPtr;
	// get the starting address of the buffer containing the block using
	// loadBlockAndGetBufferPtr(&bufferPtr).
	auto res = loadBlockAndGetBufferPtr( &bufferPtr );
	// if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
	// return the value returned by the call.
	if ( res != SUCCESS ) {
		return res;
	}
	// cast bufferPtr to type HeadInfo*
	struct HeadInfo* bufferHeader = ( struct HeadInfo* )bufferPtr;

	// copy the fields of the HeadInfo pointed to by head (except reserved) to
	// the header of the block (pointed to by bufferHeader)
	//(hint: bufferHeader->numSlots = head->numSlots )
	std::copy( head, head + 1, bufferHeader );

	// update dirty bit by calling StaticBuffer::setDirtyBit()
	// if setDirtyBit() failed, return the error code
	res = StaticBuffer::setDirtyBit( blockNum );
	if ( res != SUCCESS )
		return res;

	// return SUCCESS;
	return SUCCESS;
}

int BlockBuffer::setBlockType( int blockType ) {

	unsigned char* bufferPtr;
	/* get the starting address of the buffer containing the block
	   using loadBlockAndGetBufferPtr(&bufferPtr). */

	auto res = loadBlockAndGetBufferPtr( &bufferPtr );
	// if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
	// return the value returned by the call.
	if ( res != SUCCESS )
		return res;

	// store the input block type in the first 4 bytes of the buffer.
	// (hint: cast bufferPtr to int32_t* and then assign it)
	// *((int32_t *)bufferPtr) = blockType;
	*( reinterpret_cast<int32_t*>( bufferPtr ) ) = blockType;

	// update the StaticBuffer::blockAllocMap entry corresponding to the
	// object's block number to `blockType`.
	StaticBuffer::blockAllocMap[ blockNum ] = blockType;

	// update dirty bit by calling StaticBuffer::setDirtyBit()
	res = StaticBuffer::setDirtyBit( blockNum );

	// if setDirtyBit() failed
	// return the returned value from the call
	if ( res != SUCCESS )
		return res;

	return SUCCESS;
}

int BlockBuffer::getFreeBlock( int blockType ) {

	// iterate through the StaticBuffer::blockAllocMap and find the block number
	// of a free block in the disk.
	blockNum = -1;
	for ( int block = 0; block < DISK_BLOCKS; block++ ) {
		if ( StaticBuffer::blockAllocMap[ block ] == UNUSED_BLK ) {
			blockNum = block;
			break;
		}
	}

	// if no block is free, return E_DISKFULL.
	if ( blockNum == -1 )
		return E_DISKFULL;

	// set the object's blockNum to the block number of the free block.
	// StaticBuffer::blockAllocMap[ blockNum ] = blockType;

	// find a free buffer using StaticBuffer::getFreeBuffer() .
	auto res = StaticBuffer::getFreeBuffer( blockNum );

	// initialize the header of the block passing a struct HeadInfo with values
	// pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
	// to the setHeader() function.
	HeadInfo headInit;
	headInit.pblock		= -1;
	headInit.rblock		= -1;
	headInit.lblock		= -1;
	headInit.numEntries = 0;
	headInit.numAttrs = 0;
	headInit.numSlots = 0;
	headInit.blockType	= blockType;

	assert_res( this->setHeader( &headInit ), SUCCESS );

	// update the block type of the block to the input block type using
	// setBlockType().
	assert_res( this->setBlockType( blockType ), SUCCESS );

	// return block number of the free block.
	return blockNum;
}

int RecBuffer::setSlotMap( unsigned char* slotMap ) {
	unsigned char* bufferPtr;
	/* get the starting address of the buffer containing the block using
	   loadBlockAndGetBufferPtr(&bufferPtr). */
	auto res = loadBlockAndGetBufferPtr( &bufferPtr );

	// if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
	// return the value returned by the call.
	if ( res != SUCCESS )
		return res;

	// get the header of the block using the getHeader() function
	HeadInfo header;
	assert_res( this->getHeader( &header ), SUCCESS );

	int numSlots = header.numSlots; /* the number of slots in the block */

	// the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
	// argument `slotMap` to the buffer replacing the existing slotmap.
	// Note that size of slotmap is `numSlots`
	unsigned char* slotMapBufferPtr = bufferPtr + HEADER_SIZE;
	std::copy( slotMap, slotMap + numSlots, slotMapBufferPtr );

	// update dirty bit using StaticBuffer::setDirtyBit
	// if setDirtyBit failed, return the value returned by the call
	assert_res( StaticBuffer::setDirtyBit( this->blockNum ), SUCCESS );

	return SUCCESS;
}

void BlockBuffer::releaseBlock( ) {
	if ( this->blockNum != INVALID_BLOCKNUM && StaticBuffer::blockAllocMap[ blockNum ] != UNUSED_BLK ) {
		auto buff = StaticBuffer::getBufferNum( this->blockNum );
		if ( buff != E_BLOCKNOTINBUFFER ) {
			StaticBuffer::metainfo[ buff ].free = true;
		}
		StaticBuffer::blockAllocMap[ blockNum ] = UNUSED_BLK;
		this->blockNum							= INVALID_BLOCKNUM;
	}
}
BlockBuffer::BlockBuffer( char blockType ) {
	// allocate a block on the disk and a buffer in memory to hold the new block
	// of given type using getFreeBlock function and get the return error codes if
	// any.
	int blockTypeInt;
	if ( blockType == 'R' ) {
		blockTypeInt = REC;
	} else if ( blockType == 'I' ) {
		blockTypeInt = IND_INTERNAL;
	} else if ( blockType == 'L' ) {
		blockTypeInt = IND_LEAF;
	} else {
		std::cout << "Invalid Bloktype" << '\n';
		exit( -1 );
	}
	int blockNum = getFreeBlock( blockTypeInt );

	// set the blockNum field of the object to that of the allocated block
	// number if the method returned a valid block number,
	// otherwise set the error code returned as the block number.
	this->blockNum = blockNum;

	// (The caller must check if the constructor allocatted block successfully
	// by checking the value of block number field.)
}
// call the corresponding parent constructor
IndBuffer::IndBuffer( char blockType ) : BlockBuffer( blockType ) {}
IndBuffer::IndBuffer( int blockNum ) : BlockBuffer( blockNum ) {}

IndInternal::IndInternal( ) : IndBuffer( 'I' ) {}
IndInternal::IndInternal( int blockNum ) : IndBuffer( blockNum ) {}
// call the corresponding parent constructor
int IndInternal::getEntry( void* ptr, int indexNum ) {
	// if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
	//     return E_OUTOFBOUND.
	if ( indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL )
		return E_OUTOFBOUND;

	unsigned char* bufferPtr;
	/* get the starting address of the buffer containing the block
	   using loadBlockAndGetBufferPtr(&bufferPtr). */
	auto res = loadBlockAndGetBufferPtr( &bufferPtr );

	// if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
	//     return the value returned by the call.
	if ( res != SUCCESS )
		return res;

	// typecast the void pointer to an internal entry pointer
	struct InternalEntry* internalEntry = ( struct InternalEntry* )ptr;

	/*
	- copy the entries from the indexNum`th entry to *internalEntry
	- make sure that each field is copied individually as in the following code
	- the lChild and rChild fields of InternalEntry are of type int32_t
	- int32_t is a type of int that is guaranteed to be 4 bytes across every
	  C++ implementation. sizeof(int32_t) = 4
	*/

	/* the indexNum'th entry will begin at an offset of
	   HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
	   from bufferPtr */
	unsigned char* entryPtr = bufferPtr + HEADER_SIZE + ( indexNum * 20 );

	memcpy( &( internalEntry->lChild ), entryPtr, sizeof( int32_t ) );
	memcpy( &( internalEntry->attrVal ), entryPtr + 4, sizeof( Attribute ) );
	memcpy( &( internalEntry->rChild ), entryPtr + 20, 4 );

	return SUCCESS;
	// return SUCCESS.
}

int IndInternal::setEntry( void* ptr, int indexNum ) { 
	if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
		return E_OUTOFBOUND;
	
	unsigned char * bufferPtr;
	auto res = loadBlockAndGetBufferPtr(&bufferPtr);
	if(res != SUCCESS)
		return res;

	unsigned char* entryPtr = bufferPtr +HEADER_SIZE +(indexNum * INTERNAL_ENTRY_SIZE);
	InternalEntry* castedPtr = static_cast<InternalEntry*>(ptr);
	std::copy( castedPtr, castedPtr + 1, (InternalEntry*)entryPtr);

	assert_res(StaticBuffer::setDirtyBit(blockNum), SUCCESS);
	return SUCCESS;
}

// 'L' used to denote IndLeaf.
IndLeaf::IndLeaf( ) : IndBuffer( 'L' ) {} // this is the way to call parent non-default constructor.
IndLeaf::IndLeaf( int blockNum ) : IndBuffer( blockNum ) {}

// //this is the way to call parent non-default constructor.

int IndLeaf::setEntry( void* ptr, int indexNum ) { 
	if(indexNum < 0 || indexNum >= MAX_KEYS_LEAF) 
		return E_OUTOFBOUND;
	
	unsigned char * bufferPtr;
	auto res = loadBlockAndGetBufferPtr(&bufferPtr);
	if(res != SUCCESS)
		return res;

	unsigned char* entryPtr = bufferPtr +HEADER_SIZE +(indexNum * LEAF_ENTRY_SIZE);
	Index* castedPtr = static_cast<Index*>(ptr);
	std::copy( castedPtr, castedPtr + 1, (Index*)entryPtr);

	assert_res(StaticBuffer::setDirtyBit(blockNum), SUCCESS);
	return SUCCESS;
}
int IndLeaf::getEntry( void* ptr, int indexNum ) {

	// if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
	//     return E_OUTOFBOUND.
	if ( indexNum < 0 || indexNum >= MAX_KEYS_LEAF )
		return E_OUTOFBOUND;

	unsigned char* bufferPtr;
	/* get the starting address of the buffer containing the block
	   using loadBlockAndGetBufferPtr(&bufferPtr). */
	auto res = loadBlockAndGetBufferPtr( &bufferPtr );

	// if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
	//     return the value returned by the call.
	if ( res != SUCCESS )
		return res;

	// copy the indexNum'th Index entry in buffer to memory ptr using memcpy

	/* the indexNum'th entry will begin at an offset of
	   HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
	unsigned char* entryPtr = bufferPtr + HEADER_SIZE + ( indexNum * LEAF_ENTRY_SIZE );
	memcpy( ( struct Index* )ptr, entryPtr, LEAF_ENTRY_SIZE );

	// return SUCCESS
	return SUCCESS;
}

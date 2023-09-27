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
	int bufferNum = StaticBuffer::getBufferNum( this->blockNum );

	if ( bufferNum != E_BLOCKNOTINBUFFER ) {
		for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
			if ( StaticBuffer::metainfo[ bufferIndex ].free == false ) {
				StaticBuffer::metainfo[ bufferIndex ].timeStamp++;
			}
		}
		StaticBuffer::metainfo[ bufferNum ].timeStamp = 0;
	} else {
		std::cout << "Reading block " << blockNum << '\n';
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
	blockNum = DISK_BLOCKS;
	for ( int block = 0; block < DISK_BLOCKS; block++ ) {
		if ( StaticBuffer::blockAllocMap[ block ] == UNUSED_BLK ) {
			blockNum = block;
			break;
		}
	}

	// if no block is free, return E_DISKFULL.
	if ( blockNum == DISK_BLOCKS )
		return E_DISKFULL;

	// set the object's blockNum to the block number of the free block.
	StaticBuffer::blockAllocMap[ blockNum ] = blockType;

	// find a free buffer using StaticBuffer::getFreeBuffer() .
	auto res = StaticBuffer::getFreeBuffer( blockNum );

	// initialize the header of the block passing a struct HeadInfo with values
	// pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
	// to the setHeader() function.
	HeadInfo headInit{
		blockType, // blocktype
		-1,		   // pblock
		-1,		   // lblock
		-1,		   // rblock
		0,		   // numEntries
		0,		   // numAttrs
		0,		   // numSlots
				   // RESERVED
	};
	this->setHeader( &headInit );

	// update the block type of the block to the input block type using
	// setBlockType().
	this->setBlockType( blockType );

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
	;

	// the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
	// argument `slotMap` to the buffer replacing the existing slotmap.
	// Note that size of slotmap is `numSlots`
	auto slotMapBufferPtr = bufferPtr + HEADER_SIZE;
	std::copy( slotMap, slotMap + numSlots, slotMapBufferPtr );

	// update dirty bit using StaticBuffer::setDirtyBit
	// if setDirtyBit failed, return the value returned by the call
	StaticBuffer::setDirtyBit( this->blockNum );

	return SUCCESS;
}

void BlockBuffer::releaseBlock( ) {
	if ( this->blockNum != INVALID_BLOCKNUM ) {
		auto buff = StaticBuffer::getBufferNum( this->blockNum );
		if ( buff != E_BLOCKNOTINBUFFER ) {
			StaticBuffer::metainfo[ buff ].free = true;
		}
		StaticBuffer::blockAllocMap[ buff ] = UNUSED_BLK;
		this->blockNum = INVALID_BLOCKNUM;
	}
}

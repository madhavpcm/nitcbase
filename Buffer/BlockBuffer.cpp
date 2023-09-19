#include "BlockBuffer.h"
#include "../define/constants.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>

BlockBuffer::BlockBuffer( char blockType ) {}
BlockBuffer::BlockBuffer( int num ) : blockNum( num ) {}

RecBuffer::RecBuffer( int num ) : BlockBuffer::BlockBuffer( num ) {}

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
	std::unique_ptr<HeadInfo> head = std::make_unique<HeadInfo>( );
	this->getHeader( head.get( ) );
	int attrCount = head->numAttrs;
	int slotCount = head->numSlots;

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

		Disk::readBlock( StaticBuffer::blocks[ freeBuffer ].data( ), this->blockNum );
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

	struct HeadInfo head;
	// get the header of the block using getHeader() function
	this->getHeader( &head );

	int slotCount = head.numSlots;

	// get a pointer to the beginning of the slotmap in memory by offsetting
	// HEADER_SIZE
	unsigned char* slotMapInBuffer = bufferPtr + HEADER_SIZE;

	// copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
	std::copy( slotMapInBuffer, slotMapInBuffer + SLOTMAPSIZE( head.numAttrs ), slotMap );
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
	getHeader( &headerInfo );

	size_t numAttributes = headerInfo.numAttrs;
	size_t numSlots		 = headerInfo.numSlots;

	if ( slotNum < 0 || slotNum >= numSlots )
		return E_OUTOFBOUND;

	uint8_t* offsetPtr =
		bufferPtr + ( size_t )HEADER_SIZE + numSlots + slotNum * ( numAttributes * sizeof( union Attribute ) );
	// std::cout << std::copy( reinterpret_cast<uint8_t*>( rec ),
	//	reinterpret_cast<uint8_t*>( rec ) + numAttributes * sizeof( union Attribute ), offsetPtr ) - offsetPtr << '\n';
	std::memcpy( offsetPtr, rec, numAttributes * ATTR_SIZE);

	StaticBuffer::setDirtyBit( this->blockNum );

	return SUCCESS;
}

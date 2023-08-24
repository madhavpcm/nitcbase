#include "BlockBuffer.h"
#include "../define/constants.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer( char blockType ) {}
BlockBuffer::BlockBuffer( int num ) : blockNum( num ) {}

RecBuffer::RecBuffer( int num ) : BlockBuffer::BlockBuffer( num ) {}

int BlockBuffer::getHeader( struct HeadInfo* head ) {
	unsigned char* buffer;
	int ret = this->loadBlockAndGetBufferPtr( &buffer );
	if ( ret != SUCCESS )
		return ret;

	Disk::readBlock( buffer, blockNum );
	std::copy( buffer, buffer + sizeof( HeadInfo ), reinterpret_cast<unsigned char*>( head ) );

	return SUCCESS;
}

/*
 * @param rec
 * @param slotNum
 */
int RecBuffer::getRecord( union Attribute* rec, int slotNum ) {
	struct HeadInfo* head = new HeadInfo;
	this->getHeader( head );
	int attrCount = head->numAttrs;
	int slotCount = head->numSlots;

	// read the block at this.blockNum into a buffer
	unsigned char* buffer;
	int ret = this->loadBlockAndGetBufferPtr( &buffer );
	if ( ret != SUCCESS )
		return ret;

	Disk::readBlock( buffer, blockNum );

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

	if ( bufferNum == E_BLOCKNOTINBUFFER ) {
		bufferNum = StaticBuffer::getFreeBuffer( this->blockNum );
		if ( bufferNum == E_OUTOFBOUND ) {
			return E_OUTOFBOUND;
		}
		Disk::readBlock( StaticBuffer::blocks[ bufferNum ].data( ), this->blockNum );
	}

	// store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
	*buffer = StaticBuffer::blocks[ bufferNum ].data( );

	return SUCCESS;
}

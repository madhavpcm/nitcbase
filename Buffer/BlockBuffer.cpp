#include "BlockBuffer.h"
#include "../define/constants.h"
#include <array>
#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer( char blockType ) {}
BlockBuffer::BlockBuffer( int num ) : blockNum( num ) {}

RecBuffer::RecBuffer( int num ) : BlockBuffer::BlockBuffer( num ) {}

int BlockBuffer::getHeader( struct HeadInfo* head ) {
	std::array<unsigned char, BLOCK_SIZE> buffer;

	Disk::readBlock( buffer.data( ), blockNum );
	std::copy( buffer.begin( ), buffer.begin( ) + sizeof( HeadInfo ), reinterpret_cast<unsigned char*>( head ) );

	return SUCCESS;
}

int RecBuffer::getRecord( union Attribute* rec, int slotNum ) {
	struct HeadInfo* head = new HeadInfo;
	this->getHeader( head );
	int attrCount = head->numAttrs;
	int slotCount = head->numSlots;

	// read the block at this.blockNum into a buffer
	std::array<unsigned char, BLOCK_SIZE> buffer;
	Disk::readBlock( buffer.data( ), blockNum );

	/* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize
	   * slotNum)
	   - each record will have size attrCount * ATTR_SIZE
	   - slotMap will be of size slotCount
	   */
	int recordSize			   = attrCount * ATTR_SIZE;
	unsigned char* slotPointer = buffer.begin( ) + HEADER_SIZE + SLOTMAPSIZE( attrCount ) + recordSize * slotNum;

	// load the record into the rec data structure
	std::copy( slotPointer, slotPointer + recordSize, reinterpret_cast<unsigned char*>( rec ) );
	return SUCCESS;
}

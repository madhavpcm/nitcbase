#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

std::array<std::array<unsigned char, BLOCK_SIZE>, BUFFER_CAPACITY> StaticBuffer::blocks;
struct BufferMetaInfo StaticBuffer::metainfo[ BUFFER_CAPACITY ];
std::array<unsigned char, DISK_BLOCKS> StaticBuffer::blockAllocMap;

StaticBuffer::StaticBuffer( ) {
	Disk::readBlock( blockAllocMap.data( ), 0 );
	Disk::readBlock( blockAllocMap.data( ) + 1 * BLOCK_SIZE, 1 );
	Disk::readBlock( blockAllocMap.data( ) + 2 * BLOCK_SIZE, 2 );
	Disk::readBlock( blockAllocMap.data( ) + 3 * BLOCK_SIZE, 3 );

	// initialise all blocks as free
	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		this->metainfo[ bufferIndex ].free		= true;
		this->metainfo[ bufferIndex ].dirty		= false;
		this->metainfo[ bufferIndex ].timeStamp = -1;
		this->metainfo[ bufferIndex ].blockNum	= -1;
	}
}

StaticBuffer::~StaticBuffer( ) {
	Disk::writeBlock( blockAllocMap.data( ), 0 );
	Disk::writeBlock( blockAllocMap.data( ) + 1 * BLOCK_SIZE, 1 );
	Disk::writeBlock( blockAllocMap.data( ) + 2 * BLOCK_SIZE, 2 );
	Disk::writeBlock( blockAllocMap.data( ) + 3 * BLOCK_SIZE, 3 );

	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		if ( this->metainfo[ bufferIndex ].dirty == true ) {
			Disk::writeBlock( blocks[ bufferIndex ].data( ), this->metainfo[ bufferIndex ].blockNum );
		}
	}
}

int StaticBuffer::getFreeBuffer( int blockNum ) {
	if ( blockNum < 0 || blockNum > DISK_BLOCKS ) {
		return E_OUTOFBOUND;
	}
	int largestTimeIndex = -1;
	int largestTime		 = -1;
	for ( int block = 0; block < BUFFER_CAPACITY; block++ ) {
		if ( metainfo[ block ].free == false ) {
			metainfo[ block ].timeStamp++;
			if ( metainfo[ block ].timeStamp >= largestTimeIndex ) {
				largestTimeIndex = block;
				largestTime		 = metainfo[ block ].timeStamp;
			}
		}
	}
	int bufferNum = -1;
	for ( int block = 0; block < BUFFER_CAPACITY; block++ ) {
		if ( metainfo[ block ].free == true ) {
			bufferNum = block;
			break;
		}
	}
	if ( bufferNum == -1 ) {
		if ( metainfo[ largestTimeIndex ].dirty ) {
			Disk::writeBlock( blocks[ largestTimeIndex ].data( ), metainfo[ largestTimeIndex ].blockNum );
		}
		bufferNum = largestTimeIndex;
	}

	metainfo[ bufferNum ].dirty		= false;
	metainfo[ bufferNum ].free		= false;
	metainfo[ bufferNum ].timeStamp = 0;
	metainfo[ bufferNum ].blockNum	= blockNum;

	return bufferNum;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum( int blockNum ) {
	// Check if blockNum is valid (between zero and DISK_BLOCKS)
	// and return E_OUTOFBOUND if not valid.
	if ( blockNum < 0 || blockNum > DISK_BLOCKS )
		return E_OUTOFBOUND;

	for ( int static_buffer_index = 0; static_buffer_index < BUFFER_CAPACITY; static_buffer_index++ ) {
		if ( !metainfo[ static_buffer_index ].free && metainfo[ static_buffer_index ].blockNum == blockNum ) {
			return static_buffer_index;
		}
	}

	return E_BLOCKNOTINBUFFER;
}
int StaticBuffer::setDirtyBit( int blockNum ) {
	// find the buffer index corresponding to the block using getBufferNum().
	if ( blockNum < 0 || blockNum >= DISK_BLOCKS )
		return E_OUTOFBOUND;

	int targetIndex = -1;
	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		if ( !metainfo[ bufferIndex ].free && metainfo[ bufferIndex ].blockNum == blockNum ) {
			targetIndex = bufferIndex;
			break;
		}
	}
	if ( targetIndex == -1 )
		return E_BLOCKNOTINBUFFER;

	metainfo[ targetIndex ].dirty = true;
	return SUCCESS;
}

int StaticBuffer::getStaticBlockType( int blockNum ) {
	// Check if blockNum is valid (non zero and less than number of disk blocks)
	// and return E_OUTOFBOUND if not valid.
	if ( blockNum < 0 || blockNum >= DISK_BLOCKS )
		return E_OUTOFBOUND;

	// Access the entry in block allocation map corresponding to the blockNum
	// argument and return the block type after type casting to integer.

	return ( int32_t )blockAllocMap[ blockNum ];
}

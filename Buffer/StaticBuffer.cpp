#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

std::array<std::array<unsigned char, BLOCK_SIZE>, BUFFER_CAPACITY> StaticBuffer::blocks;
struct BufferMetaInfo StaticBuffer::metainfo[ BUFFER_CAPACITY ];
unsigned char StaticBuffer::blockAllocMap[ DISK_BLOCKS ];

StaticBuffer::StaticBuffer( ) {

	// initialise all blocks as free
	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		this->metainfo[ bufferIndex ].free		= true;
		this->metainfo[ bufferIndex ].dirty		= false;
		this->metainfo[ bufferIndex ].timeStamp = -1;
		this->metainfo[ bufferIndex ].blockNum	= -1;
	}
}

StaticBuffer::~StaticBuffer( ) {
	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		if ( this->metainfo[ bufferIndex ].free == false && this->metainfo[ bufferIndex ].dirty == true ) {
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

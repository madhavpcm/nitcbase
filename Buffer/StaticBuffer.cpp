#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"


std::array<std::array<unsigned char, BLOCK_SIZE>, BUFFER_CAPACITY> StaticBuffer::blocks;
struct BufferMetaInfo StaticBuffer::metainfo[ BUFFER_CAPACITY ];
unsigned char StaticBuffer::blockAllocMap[ DISK_BLOCKS ];

StaticBuffer::StaticBuffer( ) {

	// initialise all blocks as free
	for ( int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
		this->metainfo[ bufferIndex ].free = true;
	}
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer( ) {}

int StaticBuffer::getFreeBuffer( int blockNum ) {
	if ( blockNum < 0 || blockNum > DISK_BLOCKS ) {
		return E_OUTOFBOUND;
	}
	int allocatedBuffer;

	// iterate through all the blocks in the StaticBuffer
	// find the first free block in the buffer (check metainfo)
	// assign allocatedBuffer = index of the free block
	for ( int block = 0; block < BUFFER_CAPACITY; block++ ) {
		if ( metainfo[ block ].free == true ) {
			allocatedBuffer = block;
			break;
		}
	}

	metainfo[ allocatedBuffer ].free	 = false;
	metainfo[ allocatedBuffer ].blockNum = blockNum;

	return allocatedBuffer;
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
		if ( !metainfo[static_buffer_index].free && metainfo[ static_buffer_index ].blockNum == blockNum ) {
			return static_buffer_index;
		}
	}

	return E_BLOCKNOTINBUFFER;
}

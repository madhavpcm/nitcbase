#ifndef NITCBASE_STATICBUFFER_H
#define NITCBASE_STATICBUFFER_H

#include "../Disk_Class/Disk.h"
#include "../define/constants.h"
#include <array>
#include <memory>

struct BufferMetaInfo {
	bool free;
	bool dirty;
	int blockNum;
	int timeStamp;
};

class StaticBuffer {
	friend class BlockBuffer;

  private:
	// fields
	static struct BufferMetaInfo metainfo[ BUFFER_CAPACITY ];
	static std::array<unsigned char, DISK_BLOCKS> blockAllocMap;
	static std::array<std::array<unsigned char, BLOCK_SIZE>, BUFFER_CAPACITY> blocks;

	// methods
	static int getFreeBuffer( int blockNum );
	static int getBufferNum( int blockNum );

  public:
	// methods
	static int getStaticBlockType( int blockNum );
	static int setDirtyBit( int blockNum );
	StaticBuffer( );
	~StaticBuffer( );
};

#endif // NITCBASE_STATICBUFFER_H

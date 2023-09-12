#ifndef NITCBASE_ID_H
#define NITCBASE_ID_H
#include <initializer_list>

// for any opened relation
typedef int relId;

/* A record is identified by its block number and slot number */
struct RecId {
	int block;
	int slot;
	bool operator==( const RecId& other ) const { return ( block == other.block && slot == other.slot ); }
	bool operator!=( const RecId& other ) const { return !( block == other.block && slot == other.slot ); }
};

/* An index is identified by its block number and index number */
struct IndexId {
	int block;
	int index;
	bool operator==( const IndexId& other ) const { return ( block == other.block && index == other.index ); }
	bool operator!=( const IndexId& other ) const { return !( block == other.block && index == other.index ); }
};

#endif // NITCBASE_ID_H

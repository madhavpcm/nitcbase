#include "constants.h"
#include <cassert>

const char* getErrorDescription( int errorType ) {
	switch ( errorType ) {
	case SUCCESS:
		return "Success: The operation was successful.";
	case FAILURE:
		return "Failure: The operation failed.";
	case EXIT:
		return "Exit: The program is exiting.";
	case E_OUTOFBOUND:
		return "Out of Bound: Index or array out of bounds.";
	case E_FREESLOT:
		return "Free Slot: No free slot available.";
	case E_NOINDEX:
		return "No Index: Index not found.";
	case E_DISKFULL:
		return "Disk Full: Insufficient space in the disk.";
	case E_INVALIDBLOCK:
		return "Invalid Block: The block is invalid.";
	case E_RELNOTEXIST:
		return "Relation Not Exist: The relation does not exist.";
	case E_RELEXIST:
		return "Relation Exist: The relation already exists.";
	case E_ATTRNOTEXIST:
		return "Attribute Not Exist: The attribute does not exist.";
	case E_ATTREXIST:
		return "Attribute Exist: The attribute already exists.";
	case E_CACHEFULL:
		return "Cache Full: The cache is full.";
	case E_RELNOTOPEN:
		return "Relation Not Open: The relation is not open.";
	case E_NATTRMISMATCH:
		return "Attribute Mismatch: Mismatch in the number of attributes.";
	case E_DUPLICATEATTR:
		return "Duplicate Attribute: Duplicate attributes found.";
	case E_RELOPEN:
		return "Relation Open: The relation is already open.";
	case E_ATTRTYPEMISMATCH:
		return "Attribute Type Mismatch: Mismatch in attribute type.";
	case E_INVALID:
		return "Invalid Argument: Invalid index or argument.";
	case E_MAXRELATIONS:
		return "Maximum Relations: Maximum number of relations already present.";
	case E_MAXATTRS:
		return "Maximum Attributes: Maximum number of attributes allowed for a "
			   "relation is 125.";
	case E_NOTPERMITTED:
		return "Not Permitted: Operation not permitted.";
	case E_NOTFOUND:
		return "Not Found: Search for the requested record was unsuccessful.";
	case E_BLOCKNOTINBUFFER:
		return "Block Not Found: Block not found in buffer.";
	case E_INDEX_BLOCKS_RELEASED:
		return "Index Blocks Released: Due to insufficient disk space, index "
			   "blocks have been released from the disk.";
	default:
		return "Unknown Error: An unknown error occurred.";
	}
}

void assert_res( int actual, int expected ) {
	if ( actual != expected ) {
		std::cout << "Assertion failed: Expected return type '" << getErrorDescription( expected ) << "', but got '"
				  << getErrorDescription( actual ) << "'" << '\n';
	}
	assert( actual == expected );
}

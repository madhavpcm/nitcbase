#include "Schema.h"
#include "../BlockAccess/BlockAccess.h"
#include <cmath>
#include <cstring>
#include <unordered_set>

int Schema::openRel( char relName[ ATTR_SIZE ] ) {
	int ret = OpenRelTable::openRel( relName );

	// the OpenRelTable::openRel() function returns the rel-id if successful
	// a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
	// error codes will be negative
	if ( ret >= 0 && ret < MAX_OPEN ) {
		return SUCCESS;
	}

	// otherwise it returns an error message
	return ret;
}

int Schema::closeRel( char relName[ ATTR_SIZE ] ) {
	if ( std::strcmp( relName, "ATTRIBUTECAT" ) == 0 || std::strcmp( relName, "RELATIONCAT" ) == 0 ) {
		return E_NOTPERMITTED;
	}

	// this function returns the rel-id of a relation if it is open or
	// E_RELNOTOPEN if it is not. we will implement this later.
	int relId = OpenRelTable::getRelId( relName );

	if ( relId == E_RELNOTOPEN )
		return relId;

	return OpenRelTable::closeRel( relId );
}

int Schema::renameRel( char oldRelName[ ATTR_SIZE ], char newRelName[ ATTR_SIZE ] ) {
	if ( std::strcmp( oldRelName, "ATTRIBUTECAT" ) == 0 || std::strcmp( newRelName, "ATTRIBUTECAT" ) == 0 ||
		 std::strcmp( oldRelName, "RELATIONCAT" ) == 0 || std::strcmp( newRelName, "RELATIONCAT" ) == 0 )
		return E_NOTPERMITTED;

	if ( OpenRelTable::getRelId( oldRelName ) != E_RELNOTOPEN )
		return E_RELOPEN;

	return BlockAccess::renameRelation( ( char* )oldRelName, ( char* )newRelName );
}

int Schema::renameAttr( char relName[ ATTR_SIZE ], char oldAttrName[ ATTR_SIZE ], char newAttrName[ ATTR_SIZE ] ) {
	if ( std::strcmp( relName, "ATTRIBUTECAT" ) == 0 || std::strcmp( relName, "RELATIONCAT" ) == 0 )
		return E_NOTPERMITTED;

	if ( OpenRelTable::getRelId( relName ) != E_RELNOTOPEN )
		return E_RELOPEN;

	return BlockAccess::renameAttribute( relName, oldAttrName, newAttrName );
}

int Schema::createRel( char relName[], int nAttrs, char attrs[][ ATTR_SIZE ], int attrtype[] ) {

	// declare variable relNameAsAttribute of type Attribute
	// copy the relName into relNameAsAttribute.sVal
	union Attribute relNameAsAttribute;
	std::strcpy( relNameAsAttribute.sVal, relName );

	// declare a variable targetRelId of type RecId
	RecId targetRelId{ -1, -1 };

	// Reset the searchIndex using RelCacheTable::resetSearhIndex()
	// Search the relation catalog (relId given by the constant RELCAT_RELID)
	// for attribute value attribute "RelName" = relNameAsAttribute using
	// BlockAccess::linearSearch() with OP = EQ
	char relcat_relname[] = "RelName";
	RelCacheTable::resetSearchIndex( RELCAT_RELID );
	targetRelId = BlockAccess::linearSearch( RELCAT_RELID, relcat_relname, relNameAsAttribute, EQ );

	// if a relation with name `relName` already exists  ( linearSearch() does
	//                                                     not return {-1,-1} )
	//     return E_RELEXIST;
	if ( targetRelId != RecId{ -1, -1 } )
		return E_RELEXIST;

	// compare every pair of attributes of attrNames[] array
	// if any attribute names have same string value,
	//     return E_DUPLICATEATTR (i.e 2 attributes have same value)
	for ( int i = 0; i < nAttrs; i++ ) {
		for ( int j = i + 1; j < nAttrs; j++ ) {
			if ( std::strcmp( attrs[ i ], attrs[ j ] ) == 0 ) {
				return E_DUPLICATEATTR;
			}
		}
	}

	/* declare relCatRecord of type Attribute which will be used to store the
	   record corresponding to the new relation which will be inserted
	   into relation catalog */
	std::array<union Attribute, RELCAT_NO_ATTRS> relCatRecord;
	// fill relCatRecord fields as given below
	// offset RELCAT_REL_NAME_INDEX: relName
	std::strcpy( relCatRecord[ RELCAT_REL_NAME_INDEX ].sVal, relName );
	// offset RELCAT_NO_ATTRIBUTES_INDEX: numOfAttributes
	relCatRecord[ RELCAT_NO_ATTRIBUTES_INDEX ].nVal = nAttrs;
	// offset RELCAT_NO_RECORDS_INDEX: 0
	relCatRecord[ RELCAT_NO_RECORDS_INDEX ].nVal = 0;
	// offset RELCAT_FIRST_BLOCK_INDEX: -1
	relCatRecord[ RELCAT_FIRST_BLOCK_INDEX ].nVal = -1;
	// offset RELCAT_LAST_BLOCK_INDEX: -1
	relCatRecord[ RELCAT_LAST_BLOCK_INDEX ].nVal = -1;
	// offset RELCAT_NO_SLOTS_PER_BLOCK_INDEX: floor((2016 / (16 * nAttrs + 1)))
	// (number of slots is calculated as specified in the physical layer docs)
	relCatRecord[ RELCAT_NO_SLOTS_PER_BLOCK_INDEX ].nVal = floor( ( 2016 / ( 16 * nAttrs + 1 ) ) );

	// retVal = BlockAccess::insert(RELCAT_RELID(=0), relCatRecord);
	auto retVal = BlockAccess::insert( RELCAT_RELID, relCatRecord.data( ) );
	// if BlockAccess::insert fails return retVal
	// (this call could fail if there is no more space in the relation catalog)
	if ( retVal != SUCCESS ) {
		return retVal;
	}

	// iterate through 0 to numOfAttributes - 1 :
	for ( int i = 0; i < nAttrs; i++ ) {
		/* declare Attribute attrCatRecord[6] to store the attribute catalog
		   record corresponding to i'th attribute of the argument passed*/
		std::cout << "Inserting " << attrs[ i ] << " of type " << attrtype[ i ] << " into " << relName << '\n';
		std::array<union Attribute, ATTRCAT_NO_ATTRS> attrCatRecord;
		// (where i is the iterator of the loop)
		// fill attrCatRecord fields as given below
		// offset ATTRCAT_REL_NAME_INDEX: relName
		std::strcpy( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, relName );
		// offset ATTRCAT_ATTR_NAME_INDEX: attrNames[i]
		std::strcpy( attrCatRecord[ ATTRCAT_ATTR_NAME_INDEX ].sVal, attrs[ i ] );
		// offset ATTRCAT_ATTR_TYPE_INDEX: attrTypes[i]
		attrCatRecord[ ATTRCAT_ATTR_TYPE_INDEX ].nVal = attrtype[ i ];
		// offset ATTRCAT_PRIMARY_FLAG_INDEX: -1
		attrCatRecord[ ATTRCAT_PRIMARY_FLAG_INDEX ].nVal = -1;
		// offset ATTRCAT_ROOT_BLOCK_INDEX: -1
		attrCatRecord[ ATTRCAT_ROOT_BLOCK_INDEX ].nVal = -1;
		// offset ATTRCAT_OFFSET_INDEX: i
		attrCatRecord[ ATTRCAT_OFFSET_INDEX ].nVal = i;

		// retVal = BlockAccess::insert(ATTRCAT_RELID(=1), attrCatRecord);
		/* if attribute catalog insert fails:
		   delete the relation by calling
		   deleteRel(targetrel) of schema layer return E_DISKFULL
		// (this is necessary because we had
		   already created the
		//  relation catalog entry which needs to
		   be removed)
		*/
		retVal = BlockAccess::insert( ATTRCAT_RELID, attrCatRecord.data( ) );
		if ( retVal != SUCCESS ) {
			Schema::deleteRel( relName );
			return E_DISKFULL;
		}
	}

	// return SUCCESS
	return SUCCESS;
}
int Schema::deleteRel( char* relName ) {
	// if the relation to delete is either Relation Catalog or Attribute Catalog,
	//     return E_NOTPERMITTED
	// (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
	// you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
	if ( std::strcmp( relName, RELCAT_RELNAME ) == 0 || std::strcmp( relName, ATTRCAT_RELNAME ) == 0 )
		return E_NOTPERMITTED;

	// get the rel-id using appropriate method of OpenRelTable class by
	// passing relation name as argument
	auto relId = OpenRelTable::getRelId( relName );

	// if relation is opened in open relation table, return E_RELOPEN
	if ( relId != E_RELNOTOPEN )
		return E_RELOPEN;

	// Call BlockAccess::deleteRelation() with appropriate argument.
	// return the value returned by the above deleteRelation() call
	auto response = BlockAccess::deleteRelation( relName );
	if ( response == E_OUTOFBOUND ) {
		std::cout << "loadBlockAndGetBufferPtr sus" << '\n';
	}
	assert_res( response, SUCCESS );
	return response;

	/* the only value that should be returned from deleteRelation() is
	   E_RELNOTEXIST. The deleteRelation call may return E_OUTOFBOUND from the
	   call to loadBlockAndGetBufferPtr, but if your implementation so far has
	   been correct, it should not reach that point. That error could only occur
	   if the BlockBuffer was initialized with an invalid block number.
	*/
}

int Schema::createIndex( char relName[ ATTR_SIZE ], char attrName[ ATTR_SIZE ] ) {
	// if the relName is either Relation Catalog or Attribute Catalog,
	// return E_NOTPERMITTED
	// (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
	// you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
	if ( std::strcmp( relName, RELCAT_RELNAME ) == 0 || std::strcmp( relName, ATTRCAT_RELNAME ) == 0 )
		return E_NOTPERMITTED;

	// get the relation's rel-id using OpenRelTable::getRelId() method
	auto relId = OpenRelTable::getRelId( relName );

	// if relation is not open in open relation table, return E_RELNOTOPEN
	// (check if the value returned from getRelId function call = E_RELNOTOPEN)
	if ( relId == E_RELNOTOPEN ) {
		return relId;
	}

	// create a bplus tree using BPlusTree::bPlusCreate() and return the value
	return BPlusTree::bPlusCreate( relId, attrName );
}

int Schema::dropIndex( char* relName, char* attrName ) {
	// if the relName is either Relation Catalog or Attribute Catalog,
	// return E_NOTPERMITTED
	// (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
	// you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
	if ( std::strcmp( relName, RELCAT_RELNAME ) == 0 || std::strcmp( relName, ATTRCAT_RELNAME ) == 0 )
		return E_NOTPERMITTED;

	// get the relation's rel-id using OpenRelTable::getRelId() method
	auto relId = OpenRelTable::getRelId( relName );

	// if relation is not open in open relation table, return E_RELNOTOPEN
	// (check if the value returned from getRelId function call = E_RELNOTOPEN)
	if ( relId == E_RELNOTOPEN ) {
		return relId;
	}

	// get the attribute catalog entry corresponding to the attribute
	// using AttrCacheTable::getAttrCatEntry()
	AttrCatEntry attrCatEntry;
	auto res = AttrCacheTable::getAttrCatEntry( relId, attrName, &attrCatEntry );

	// if getAttrCatEntry() fails, return E_ATTRNOTEXIST
	if ( res != SUCCESS )
		return E_ATTRNOTEXIST;

	int rootBlock = attrCatEntry.rootBlock /* get the root block from attrcat entry */;

	if ( rootBlock == -1 /* attribute does not have an index (rootBlock = -1) */ ) {
		return E_NOINDEX;
	}

	// destroy the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
	BPlusTree::bPlusDestroy( rootBlock );

	// set rootBlock = -1 in the attribute cache entry of the attribute using
	attrCatEntry.rootBlock = -1;
	// AttrCacheTable::setAttrCatEntry()
	AttrCacheTable::setAttrCatEntry( relId, attrName, &attrCatEntry );

	return SUCCESS;
}

#include "OpenRelTable.h"
#include "AttrCacheTable.h"
#include "RelCacheTable.h"
#include <cstring>
#include <iostream>
#include <memory>

OpenRelTable::OpenRelTable( ) {

	// initialize relCache and attrCache with nullptr
	for ( int i = 0; i < MAX_OPEN; ++i ) {
		RelCacheTable::relCache[ i ]   = nullptr;
		AttrCacheTable::attrCache[ i ] = { };
	}

	/************ Setting up Relation Cache entries ************/
	// (we need to populate relation cache with entries for the relation catalog
	//  and attribute catalog.)

	/**** setting up Relation Catalog relation in the Relation Cache Table****/
	RecBuffer relCatBlock( RELCAT_BLOCK );
	std::array<union Attribute, RELCAT_NO_ATTRS> relCatRecord;
	struct RelCacheEntry relCatRecordCacheEntry;

	/*
	 * Insert relation catalog entry to cache[0]
	 */
	relCatBlock.getRecord( relCatRecord.data( ), RELCAT_SLOTNUM_FOR_RELCAT );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId.block = RELCAT_BLOCK;
	relCatRecordCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_RELCAT;

	RelCacheTable::relCache[ RELCAT_RELID ]		 = new RelCacheEntry;
	*( RelCacheTable::relCache[ RELCAT_RELID ] ) = relCatRecordCacheEntry;

	/*
	 * Insert Attribute catalog entry to cache[0]
	 */

	relCatBlock.getRecord( relCatRecord.data( ), RELCAT_SLOTNUM_FOR_ATTRCAT );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId.block = RELCAT_BLOCK;
	relCatRecordCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_ATTRCAT;

	RelCacheTable::relCache[ ATTRCAT_RELID ]	  = new RelCacheEntry;
	*( RelCacheTable::relCache[ ATTRCAT_RELID ] ) = relCatRecordCacheEntry;

	/*
	 * Insert Students catalog to cache[0] -- 4.exercise
	 */
	relCatBlock.getRecord( relCatRecord.data( ), 2 );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId.block = RELCAT_BLOCK;
	relCatRecordCacheEntry.recId.slot  = 2;
	RelCacheTable::relCache[ 2 ]	   = new RelCacheEntry;
	*( RelCacheTable::relCache[ 2 ] )  = relCatRecordCacheEntry;

	/************ Setting up Attribute cache entries ************/
	// (we need to populate attribute cache with entries for the relation catalog
	//  and attribute catalog.)

	/**** setting up Relation Catalog relation in the Attribute Cache Table ****/
	// iterate through all the attributes of the relation catalog and create a
	// linked list of AttrCacheEntry (slots 0 to 5) for each of the entries, set
	//    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
	//    attrCacheEntry.recId.slot = i   (0 to 5)
	//    and attrCacheEntry.next appropriately
	// NOTE: allocate each entry dynamically using malloc

	// set the next field in the last entry to nullptr
	RecBuffer attrCatBlock( ATTRCAT_BLOCK );
	std::array<union Attribute, ATTRCAT_NO_ATTRS> attrCatRecord;
	struct HeadInfo attrCatHeader;
	attrCatBlock.getHeader( &attrCatHeader );
	std::list<AttrCacheEntry> relCatAttrList;
	std::list<AttrCacheEntry> attrCatAttrList;
	std::list<AttrCacheEntry> studCatAttrList;

	for ( int i = 0; i < attrCatHeader.numEntries; i++ ) {
		attrCatBlock.getRecord( attrCatRecord.data( ), i );
		AttrCacheEntry attrListEntry;
		std::unique_ptr<AttrCatEntry> attrData = std::make_unique<AttrCatEntry>( );
		AttrCacheTable::recordToAttrCatEntry( attrCatRecord.data( ), attrData.get( ) );

		if ( std::strcmp( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, "RELATIONCAT" ) == 0 ) {
			attrListEntry.attrCatEntry = *attrData;
			attrListEntry.recId.block  = ATTRCAT_BLOCK;
			attrListEntry.recId.slot   = i;
			relCatAttrList.push_back( std::move( attrListEntry ) );
		}
		if ( std::strcmp( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, "ATTRIBUTECAT" ) == 0 ) {
			attrListEntry.attrCatEntry = *attrData;
			attrListEntry.recId.block  = ATTRCAT_BLOCK;
			attrListEntry.recId.slot   = i;
			attrCatAttrList.push_back( std::move( attrListEntry ) );
		}
		if ( std::strcmp( attrCatRecord[ ATTRCAT_REL_NAME_INDEX ].sVal, "students" ) == 0 ) {
			attrListEntry.attrCatEntry = *attrData;
			attrListEntry.recId.block  = ATTRCAT_BLOCK;
			attrListEntry.recId.slot   = i;
			studCatAttrList.push_back( std::move( attrListEntry ) );
		}
	}

	AttrCacheTable::attrCache[ RELCAT_RELID ]  = std::move( relCatAttrList );
	AttrCacheTable::attrCache[ ATTRCAT_RELID ] = std::move( attrCatAttrList );
	AttrCacheTable::attrCache[ 2 ]			   = std::move( studCatAttrList );

	/**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

	// set up the attributes of the attribute cache similarly.
	// read slots 6-11 from attrCatBlock and initialise recId appropriately

	// set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
}

int OpenRelTable::getRelId( char relName[ ATTR_SIZE ] ) {
	if ( std::strcmp( relName, "RELATIONCAT" ) == 0 )
		return RELCAT_RELID;

	if ( std::strcmp( relName, "ATTRIBUTECAT" ) == 0 )
		return ATTRCAT_RELID;

	if ( std::strcmp( relName, "students" ) == 0 )
		return 2;

	return E_RELNOTOPEN;
}

OpenRelTable::~OpenRelTable( ) {
	// free all the memory that you allocated in the constructor
}

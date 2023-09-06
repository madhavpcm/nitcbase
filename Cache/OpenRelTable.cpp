#include "OpenRelTable.h"
#include "AttrCacheTable.h"
#include "RelCacheTable.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>

std::array<OpenRelTableMetaInfo, MAX_OPEN> OpenRelTable::tableMetaInfo;
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
	 * Insert relation catalog entry to relcache[0]
	 */
	relCatBlock.getRecord( relCatRecord.data( ), RELCAT_SLOTNUM_FOR_RELCAT );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId.block = RELCAT_BLOCK;
	relCatRecordCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_RELCAT;

	RelCacheTable::relCache[ RELCAT_RELID ]		 = new RelCacheEntry;
	*( RelCacheTable::relCache[ RELCAT_RELID ] ) = relCatRecordCacheEntry;

	/*
	 * Insert Attribute catalog entry to relcache[1]
	 */

	relCatBlock.getRecord( relCatRecord.data( ), RELCAT_SLOTNUM_FOR_ATTRCAT );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId.block = RELCAT_BLOCK;
	relCatRecordCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_ATTRCAT;

	RelCacheTable::relCache[ ATTRCAT_RELID ]	  = new RelCacheEntry;
	*( RelCacheTable::relCache[ ATTRCAT_RELID ] ) = relCatRecordCacheEntry;
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
	}

	AttrCacheTable::attrCache[ RELCAT_RELID ]  = std::move( relCatAttrList );
	AttrCacheTable::attrCache[ ATTRCAT_RELID ] = std::move( attrCatAttrList );

	/************ Setting up tableMetaInfo entries ************/

	// in the tableMetaInfo array
	//   set free = false for RELCAT_RELID and ATTRCAT_RELID
	//   set relname for RELCAT_RELID and ATTRCAT_RELID
	OpenRelTable::tableMetaInfo[ RELCAT_RELID ].free = false;
	std::strcpy( OpenRelTable::tableMetaInfo[ RELCAT_RELID ].relName, "RELATIONCAT" );
	OpenRelTable::tableMetaInfo[ ATTRCAT_RELID ].free = false;
	std::strcpy( OpenRelTable::tableMetaInfo[ ATTRCAT_RELID ].relName, "ATTRIBUTECAT" );
	
	for(int i=2 ; i < MAX_OPEN; i++)
		tableMetaInfo[i].free = true;
}

int OpenRelTable::getRelId( char relName[ ATTR_SIZE ] ) {
	for ( int i = 0; i < MAX_OPEN; i++ ) {
		if ( std::strcmp( relName, tableMetaInfo[ i ].relName ) == 0 ) {
			return i;
		}
	}

	return E_RELNOTOPEN;
}

OpenRelTable::~OpenRelTable( ) {
	for ( int i = 2; i < MAX_OPEN; ++i ) {
		if ( !tableMetaInfo[ i ].free ) {
			OpenRelTable::closeRel( i );
		}
	}
	// free all the memory that you allocated in the constructor
}

int OpenRelTable::getFreeOpenRelTableEntry( ) {

	/* traverse through the tableMetaInfo array,
	  find a free entry in the Open Relation Table.*/
	for ( int i = 2; i < MAX_OPEN; i++ ) {
		if ( tableMetaInfo[ i ].free ) {
			tableMetaInfo[ i ].free = false;
			return i;
		}
	}

	// if found return the relation id, else return E_CACHEFULL.
	return E_CACHEFULL;
}

int OpenRelTable::closeRel( int relId ) {
	if ( relId == RELCAT_RELID || relId == ATTRCAT_RELID ) {
		return E_NOTPERMITTED;
	}

	if ( relId < 0 || relId >= MAX_OPEN ) {
		return E_OUTOFBOUND;
	}

	if ( tableMetaInfo[ relId ].free ) {
		return E_RELNOTOPEN;
	}

	// free the memory allocated in the relation and attribute caches which was
	// allocated in the OpenRelTable::openRel() function
	delete RelCacheTable::relCache[ relId ];
	RelCacheTable::relCache[ relId ] = nullptr;
	AttrCacheTable::attrCache[ relId ].clear( );

	// update `tableMetaInfo` to set `relId` as a free slot
	// update `relCache` and `attrCache` to set the entry at `relId` to nullptr
	tableMetaInfo[ relId ].free = true;
	return SUCCESS;
}

int OpenRelTable::openRel( char relName[ ATTR_SIZE ] ) {

	int exists = getRelId( relName );
	if ( exists != E_RELNOTOPEN ) {
		return exists;
	}

	/* find a free slot in the Open Relation Table
	   using OpenRelTable::getFreeOpenRelTableEntry(). */

	int relId = getFreeOpenRelTableEntry( );
	if ( relId == E_CACHEFULL ) {
		return E_CACHEFULL;
	}

	/****** Setting up Relation Cache entry for the relation ******/

	/* search for the entry with relation name, relName, in the Relation Catalog
	   using BlockAccess::linearSearch(). Care should be taken to reset the
	   searchIndex of the relation RELCAT_RELID before calling linearSearch().*/

	// relcatRecId stores the rec-id of the relation `relName` in the Relation
	// Catalog.
	RecId relcatRecId;
	RelCacheTable::resetSearchIndex( RELCAT_RELID );
	RelCacheTable::getSearchIndex( RELCAT_RELID, &relcatRecId );
	union Attribute attrVal;
	std::strcpy( attrVal.sVal, relName );
	char pass[ ATTR_SIZE ] = "RelName";

	relcatRecId = BlockAccess::linearSearch( RELCAT_RELID, pass,attrVal, EQ );

	if ( relcatRecId.slot == -1 && relcatRecId.block == -1 ) {
		// (the relation is not found in the Relation Catalog.)
		return E_RELNOTEXIST;
	}

	/*
	  read the record entry corresponding to relcatRecId and create a
	  relCacheEntry on it using RecBuffer::getRecord() and
	  RelCacheTable::recordToRelCatEntry(). update the recId field of this
	  Relation Cache entry to relcatRecId. use the Relation Cache entry to set the
	  relId-th entry of the RelCacheTable. NOTE: make sure to allocate memory for
	  the RelCacheEntry using malloc()
	*/
	RecBuffer recBuffer( relcatRecId.block );
	std::array<union Attribute, RELCAT_NO_ATTRS> relCatRecord;
	struct RelCacheEntry relCatRecordCacheEntry;

	recBuffer.getRecord( relCatRecord.data( ), relcatRecId.slot );
	RelCacheTable::recordToRelCatEntry( relCatRecord.data( ), &relCatRecordCacheEntry.relCatEntry );
	relCatRecordCacheEntry.recId		  = relcatRecId;
	RelCacheTable::relCache[ relId ]	  = new RelCacheEntry;
	*( RelCacheTable::relCache[ relId ] ) = relCatRecordCacheEntry;

	/****** Setting up Attribute Cache entry for the relation ******/

	// let listHead be used to hold the head of the linked list of attrCache
	// entries.

	/*iterate over all the entries in the Attribute Catalog corresponding to each
	attribute of the relation relName by multiple calls of
	BlockAccess::linearSearch() care should be taken to reset the searchIndex of
	the relation, ATTRCAT_RELID, corresponding to Attribute Catalog before the
	first call to linearSearch().*/
	{
		/* let attrcatRecId store a valid record id an entry of the relation,
		relName, in the Attribute Catalog.*/

		/* read the record entry corresponding to attrcatRecId and create an
		Attribute Cache entry on it using RecBuffer::getRecord() and
		AttrCacheTable::recordToAttrCatEntry().
		update the recId field of this Attribute Cache entry to attrcatRecId.
		add the Attribute Cache entry to the linked list of listHead .*/
		// NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
	}
	std::array<union Attribute, ATTRCAT_NO_ATTRS> attrCatRecord;
	std::list<AttrCacheEntry> attrCacheList;

	RelCacheTable::resetSearchIndex( ATTRCAT_RELID);
	RecId attrcatRecId = BlockAccess::linearSearch( ATTRCAT_RELID, pass, attrVal, EQ );
	while ( attrcatRecId.slot != -1 && attrcatRecId.block != -1 ) {
		RecBuffer attrBuffer( attrcatRecId.block );
		attrBuffer.getRecord(attrCatRecord.data(), attrcatRecId.slot);
		AttrCacheEntry attrListEntry;
		AttrCacheTable::recordToAttrCatEntry( attrCatRecord.data( ), &attrListEntry.attrCatEntry );
		attrListEntry.recId = attrcatRecId;
		attrCacheList.push_back(  attrListEntry  );
		attrcatRecId = BlockAccess::linearSearch( ATTRCAT_RELID, pass, attrVal, EQ );
	}

	// set the relIdth entry of the AttrCacheTable to listHead.
	AttrCacheTable::attrCache[ relId ] = std::move( attrCacheList );

	/****** Setting up metadata in the Open Relation Table for the relation******/

	// update the relIdth entry of the tableMetaInfo with free as false and
	// relName as the input.
	tableMetaInfo[ relId ].free = false;
	std::strcpy( tableMetaInfo[ relId ].relName, relName );

	return relId;
}

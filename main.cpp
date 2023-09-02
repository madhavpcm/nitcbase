#include "Buffer/BlockBuffer.h"
#include "Buffer/StaticBuffer.h"
#include "Cache/AttrCacheTable.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "define/constants.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

int main( int argc, char* argv[] ) {
	/* Initialize the Run Copy of Disk */

	//	  unsigned char buffer[BLOCK_SIZE];
	Disk disk_run;
	StaticBuffer buffer;
	OpenRelTable cache;

	/* for ( int i = 0; i < 3; i++ ) {
		std::unique_ptr<RelCatEntry> relCatEntry=std::make_unique<RelCatEntry>();
		if ( RelCacheTable::getRelCatEntry( i, relCatEntry.get() ) != SUCCESS ) {
			std::cout << "RelCat Entry Error\n";
			break;
		}
		std::cout << "Relation " << relCatEntry.get( )->relName << '\n';
		for ( int j = 0; j < relCatEntry.get( )->numAttrs; j++ ) {
			std::unique_ptr<AttrCatEntry> attrCatEntry = std::make_unique<AttrCatEntry>();
			if ( AttrCacheTable::getAttrCatEntry( i, j, attrCatEntry.get() ) != SUCCESS ) {
				continue;
			}
			std::cout << " " << attrCatEntry.get( )->attrName << ": " << attrCatEntry.get( )->attrType << '\n';
		}
	}

	return 0; */
	return FrontendInterface::handleFrontend( argc, argv );
}

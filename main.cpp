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

	return FrontendInterface::handleFrontend( argc, argv );
}

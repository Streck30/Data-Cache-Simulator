#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
struct cache{
	int dirtyBit;
	int tag;
	int lastUse;
};
int main( int argc, char ** argv )
{	
	int readOrWrite = 0;
	int dataReads = 0;
	int dataWrites = 0;
	int dataAccess = 0;
	int dataReadMiss = 0;
	int dataWriteMiss = 0;
	int dataMiss = 0;
	int dirtyReadMiss = 0;
	int dirtyWriteMiss = 0;
	int bytesRead = 0;
	int bytesWritten = 0;
	int totalReadTime = 0;
	int totalWriteTime = 0;
	int dashVee = 0;
	int dashVeeLine1 = 0;
	int dashVeeLine2 = 0;
	char bufferHolder[1024*1024*5];
	int cacheSize;
	const int blockSize = 16;
	const int blockOffset = 4;
	const int missPenalty = 80;
	int found = 0;
	if( argc != 7 && argc != 4 ) {
		printf("input arguments as file, cachesize, and number of blocks or additionally -v instruction start, instruction end\n");
		return 1;
	}
	if( argc == 7 && strcmp( argv[4], "-v") == 0 ) {
		dashVee = 1;
		dashVeeLine1 = atoi(argv[5]);
		dashVeeLine2 = atoi(argv[6]);
	}
	int numBlocks = atoi(argv[3]);
	cacheSize = atoi(argv[2]) << 10;
	int totalBlocks = cacheSize / blockSize / numBlocks;
	int blockShift = 0;
	int tmpBlocks = totalBlocks;
	do{
		tmpBlocks = tmpBlocks >> 1;
		blockShift++;
	}
	while(tmpBlocks > 1);
	int openError = open( argv[1], O_RDONLY ); 	
	if( openError == -1 ) {
		printf("File open error\n");
		return 1;
	}
	ssize_t readError = read( openError, bufferHolder, sizeof( bufferHolder ) );
	if( readError == -1 ) {
		printf( "file read error\n" );
		return 1;
	}
	struct cache dataCache[ totalBlocks ][ numBlocks ];
	for( int cacheFill = 0; cacheFill < totalBlocks; cacheFill++ ) {
		for( int cacheFill2 = 0; cacheFill2 < numBlocks; cacheFill2++ )
			dataCache[ cacheFill ][ cacheFill2 ].tag = -1;
	}
	char* endCheck = strtok( bufferHolder, ": \t\n" );
	do {
		if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
			printf("%d ",dataAccess); 
		char * readWrite = strtok( NULL, ": \t\n" );
		if( strcmp( readWrite, "W" ) == 0 ) {
			readOrWrite = 1;
			dataWrites++;
		}
		else{
			readOrWrite = 0;
			dataReads++;
		}
		char * tmpBinary = strtok( NULL, ": \t\n" );
		long currentBinary = strtol( tmpBinary, NULL, 16 );
		int currentIndex = currentBinary >> blockOffset;
		currentIndex = currentIndex & ( totalBlocks - 1 );
		if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
			printf("0x%x ",currentIndex);
		int currentTag = currentBinary >> ( blockShift + blockOffset );
		if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
			printf("0x%x ",currentTag);
		for(int checkAllBlocks = 0; checkAllBlocks < numBlocks; checkAllBlocks++ ) {
			if( dataCache[currentIndex][checkAllBlocks].tag == currentTag ) {
				found = 1;
				if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
					printf("1 %d %d 0x%x %d 1 1", checkAllBlocks, dataCache[currentIndex][checkAllBlocks].lastUse, dataCache[currentIndex][checkAllBlocks].tag, dataCache[currentIndex][checkAllBlocks].dirtyBit);
				dataCache[currentIndex][checkAllBlocks].lastUse = dataAccess;
				if(readOrWrite == 1)
					totalWriteTime++;
				else
					totalReadTime++;
				if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
					printf("1 1\n");
			}
		}
		if(found == 0) {
			int blockToReplace = 0;
			dataMiss++;
			for(int checkAllBlocks = 0; checkAllBlocks < numBlocks-1; checkAllBlocks++ ) {
				if( dataCache[currentIndex][checkAllBlocks].lastUse > dataCache[currentIndex][checkAllBlocks+1].lastUse)
					blockToReplace = checkAllBlocks+1;
			}
			if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 ) {
				if(dataCache[currentIndex][blockToReplace].tag == -1)
					printf("0 ");
				else
					printf("1 ");
				printf("%d %d 0x%x %d 0", blockToReplace, dataCache[currentIndex][blockToReplace].lastUse, dataCache[currentIndex][blockToReplace].tag, dataCache[currentIndex][blockToReplace].dirtyBit);
			}
			if( dataCache[currentIndex][blockToReplace].dirtyBit == 1 ) {
				if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
					printf("2b\n");
				if(readOrWrite == 1) {
					totalWriteTime += missPenalty;
					dirtyWriteMiss++;
				}
				else {
					totalReadTime += missPenalty;
					dirtyReadMiss++;
					dataCache[currentIndex][blockToReplace].dirtyBit = 0;
				}
			}else {
				if( dashVee == 1 && dataAccess >= dashVeeLine1 && dataAccess <= dashVeeLine2 )
					printf("2a\n");
			}
			dataCache[currentIndex][blockToReplace].tag = currentTag;
			dataCache[currentIndex][blockToReplace].lastUse = dataAccess;
			if( readOrWrite == 1 ) {
				dataCache[currentIndex][blockToReplace].dirtyBit = 1;
				totalWriteTime += missPenalty + 1;
				dataWriteMiss++; 
			}
			else {
				totalReadTime += missPenalty + 1;
				dataReadMiss++;
				dataCache[currentIndex][blockToReplace].dirtyBit = 0;
			}
		}
		char * tmpByteString = strtok( NULL, ": \t\n" );
		int tmpByte = (int)strtol( tmpByteString, NULL, 10 );
		if( readOrWrite == 1 )
			bytesWritten += tmpByte;
		else
			bytesRead += tmpByte;
		strtok( NULL, ": \t\n" );
		endCheck = strtok( NULL, ": \t\n" );
		if( endCheck == NULL )
			break;
		dataAccess++;
		found = 0;
	} while( endCheck != NULL );
	printf("Data reads: %d\nData writes: %d\nData accesses: %d\nData read misses: %d\nData write misses: %d\n", dataReads, dataWrites, dataAccess+1, dataReadMiss, dataWriteMiss);
	printf("Data misses: %d\nDirty read misses: %d\nDirty write misses: %d\nTotal bytes read: %d\nTotal bytes written: %d\n",dataMiss, dirtyReadMiss, dirtyWriteMiss, bytesRead, bytesWritten);
	printf("Total read time: %d\nTotal write time: %d\nMiss rate: %f\n",totalReadTime, totalWriteTime, (float)dataMiss / (dataAccess + 1));
	return 0;
}


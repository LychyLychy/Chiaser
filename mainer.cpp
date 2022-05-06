#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//References
/*
* https://www.robotplanet.dk/audio/wav_meta_data/ for RIFF WAVE metadata
* https://tech.ebu.ch/docs/tech/tech3285.pdf Broadcast Wave Format
* https://tech.ebu.ch/docs/r/r098.pdf BWF Coding History
* 
* https://id3.org/id3v2.4.0-frames id3 v4
* https://id3.org/id3v2.3.0 id3 v3
* https://id3.org/id3v2-00 id3 v2
*/
#define flip _byteswap_ulong
#ifndef flip
#define flip
#endif
//#define csvPath "C:\\bruh.csv"
#define metaNum 14
// Meta Data identifiers expected  IENG is repeated twice in the link for whatever reason  Flip them too because
unsigned long metaIdentifiers[metaNum] = { flip('INAM'),flip('IPRD'),flip('IART'),flip('ICRD'),flip('ITRK'),flip('ICMT'),flip('IKEY'),flip('ISFT'),flip('IENG'),flip('ITCH'),flip('IGNR'),flip('ICOP'),flip('ISBJ'),flip('ISRC') };

unsigned long id3Identifiers[3][metaNum] = {
	{ flip('TT2'),flip('TAL'),flip('TP1'),flip('TYE'),flip('TRK'),flip('COM'),flip('TXX'),flip('TEN'),flip('TP2'),flip('TP3'),flip('TCO'),flip('TCR'),flip('TPB'),flip('TOT')}, //ID3V2
	{ flip('TIT2'),flip('TALB'),flip('TPE1'),flip('TYER'),flip('TRCK'),flip('TXXX'),flip('TCON'),flip('TENC'),flip('TPE2'),flip('TPE3'),flip('TMED'),flip('TCOP'),flip('TPUB'),flip('TOAL')},
	{ flip('TIT2'),flip('TALB'),flip('TPE1'),flip('TYER'),flip('TRCK'),flip('TXXX'),flip('TCON'),flip('TENC'),flip('TPE2'),flip('TPE3'),flip('TMOO'),flip('TCOP'),flip('TPUB'),flip('TOAL')}, //ID3V4

 };



enum filetype_t
{
	fileNone,
	fileWav,
	fileMp3,
};

enum
{
	id3None, 

	id3v2 = 2,
	id3v3,
	id3v4,
};
enum
{
	META_TITLE,
	META_ALBUM,
	META_ARTIST,
	META_CREATIONDATE,
	META_TRACKNUM,
	META_COMMENT,
	META_KEYWORDS,
	META_SOFTWARE,
	META_ENGINEER,
	META_TECHNICIAN,
	META_GENRE,
	META_COPYRIGHT,
	META_SUBJECT,
	META_SOURCE,
};

int synchsafeIntToInt(int* number)
{
	int newInt(0);
	for (int j = 0; j <= 4; j++) // Conversion from synchsafe
	{
		newInt |= *((char*)(number) + 4 - j) << (7 * j);
	}
	return newInt;
}


int main(int argc, char** argv)

{
	if (argc == 3)
	{
		filetype_t filetype = fileNone;
		char* filePathLowered = _strlwr(argv[1]);
		char* yearString = 0;
		if (strstr(filePathLowered, ".wav"))
			filetype = fileWav;
		else if (strstr(filePathLowered, ".mp3"))
			filetype = fileMp3;

		else
			return 'TYPE';

			

		char* buffer;
			char* metadata[metaNum]{};
		

	
		FILE* fileHandle = fopen(argv[1], "rb");
		if (!fileHandle)
			return 'FHND';

		fseek(fileHandle, 0, SEEK_END);
		size_t fileLength = ftell(fileHandle);
		rewind(fileHandle);
		buffer = (char*)calloc(1,fileLength+256); //give it some extra space for safety
		if (!buffer)
			return 'BUFF';
		fread((char*)buffer, 1, fileLength, fileHandle);
		if (!buffer)
			return 'READ';

		bool atInfo, atList,hasId3;
		atInfo = atList = hasId3 = false;
		char id3Version = 0;
		
		int locationOfMagicNumber = 0;
		//Now READ!!!
		
		for (unsigned int i = 0; i <= fileLength; i++)
		{
			int* intSeeker = (int*)&buffer[i];

			if (filetype == fileWav) // Only wav files support BWF and RIFF metadata
			{
				if (*intSeeker == flip('bext')) //A BWF chunk ('bext' backwards ) https://tech.ebu.ch/docs/tech/tech3285.pdf at 2.3
				{
					int bextSize = *(intSeeker + 1);
					printf("BEXT chunk found. Size: %d\n", bextSize);
					//if (!(*(intSeeker + 4) > 512 || *(intSeeker + 4) <= 0))
					if (bextSize < 2048 && bextSize>0) // the size should be less than 2048
					{

						metadata[META_TITLE] = (char*)intSeeker + 8; // Description [256], basically the voice actor e.g john lowry28. Will go to title.
						metadata[META_SOFTWARE] = (char*)intSeeker + 8 + 256; // Originator [32], the software used, e.g. Pro Tools. Will go to Software
						if ((strcmp(metadata[META_SOFTWARE],"")==0))
						{
							char* codingHistoryComment = strstr((char*)(intSeeker)+610, "T="); //Coding History 610 bytes after magic number
							if (codingHistoryComment)
							{
								printf("BWF Coding History found, %s", (char*)(intSeeker)+610);
								metadata[META_SOFTWARE] = codingHistoryComment + 2;
								*strrchr(metadata[META_SOFTWARE], ',') = 0;
							}

						}
							
						char* creationDate = (char*)calloc(20, 1); // Now the creation date
						char* creationDateOffset = (char*)intSeeker + 256 + 32 + 32 + 8;
						size_t dateLength = strlen("2003-05-11");
						strncpy(creationDate, creationDateOffset, dateLength); //Only grab the date, offset of 328 from bext
						strcat(creationDate, " "); //add a space
						*(creationDateOffset + 18) = 0; //set garbage after time stamp to nul
						strcat(creationDate, creationDateOffset + dateLength);//concat the time stamp
						metadata[META_CREATIONDATE] = creationDate;
						continue;
					}
					else
						printf("Nvm fuck that, bext size: %i\n", bextSize); continue;
					

				}

				if (*intSeeker == flip('LIST'))
					atList = true;
				if (*intSeeker == flip('INFO'))
					atInfo = true;

				if (atInfo && atList) //dont even bother if we havent seen INFO yet
				{
					if (buffer[i] == 'I') //
					{
						for (int j = 0; j < metaNum; j++)
						{
							if (*intSeeker == metaIdentifiers[j])
							{
								char* valueAhead = &buffer[i] + 8; //8 bytes ahead is where the data should be
								metadata[j] = valueAhead; //Set the pointer in the metadata array to the metadata value
								break;
							}

						}
					}
				}
			}
		

			if (!hasId3)
			{
				if ( * intSeeker << 8 == '3DI\0' && *(&buffer[i] + 3)<0xFF && *(&buffer[i] + 4)<0xFF && *(&buffer[i] + 6)<0x80 && *(&buffer[i] + 7) < 0x80 && *(&buffer[i] + 8) < 0x80 && *(&buffer[i] + 9) < 0x80)
				{
					
				id3Version = *(&buffer[i] + 3); //This is the id3 version 
				if (id3Version >= id3v2 && id3Version <= id3v4) //These are the only acceptable versions
				{
					printf("ID3v%i header found\n", id3Version);
					locationOfMagicNumber = i;
					hasId3 = true;
				}
				}
			}


			if (hasId3 && i- locationOfMagicNumber <512) //Give up if we reached 512 bytes past the header
			{
				if (buffer[i] == 'T')
				{
					int id3TagSize = 0;

					//int tagLength = id3Version == id3v2 ? 3 : 4;
					
					//for (int j = 0; j < tagLength; j++) // Conversion from synchsafe
					//{ 
						//int surrogate = *((int*)((char*)(intSeeker)+tagLength - 1));
						//surrogate = flip(surrogate);
					//	if (id3Version == id3v2)
					//		((char*)(&surrogate)) [0] = 0;
					//	id3TagSize |= *((char*)(&surrogate)+(tagLength-1) - j) << (7 * j);
					//}
					id3TagSize = id3Version == id3v2 ? *(&buffer[i] + 5) : *(&buffer[i] + 7); // you can get away with one char


					if (id3TagSize < 256 && id3TagSize>5)
					{
						void* dataBuffer = calloc(1, id3TagSize + 10); //add 10 nuls to the end for safety
						char* id3Buffer = (char*)dataBuffer;
						if (id3Version == id3v2)
							memcpy(dataBuffer, (char*)(intSeeker)+6, id3TagSize);
						else
						memcpy(dataBuffer, (char*)(intSeeker) + 10, id3TagSize); //offset from frame identifier + size +flags 
						void * zeroLoc = 0;
						while (zeroLoc = memchr(dataBuffer, 0, id3TagSize))
						{
							*(char*)zeroLoc = ' ';  //Set each nul to a space 
						}


						for (int j = 0; j < metaNum; j++)
						{
							int id3Tag = 0;
							if (id3Version == id3v2)
								id3Tag = *intSeeker << 8;
							else
								id3Tag = *intSeeker;

							if (id3Tag == id3Identifiers[id3Version-2][j])
							{
								char* whereIsIt = 0;
								
								switch (id3Tag)
								{



								case('REYT'):
								case('EYT'):
									yearString = (char*)malloc(10);
									strcpy(yearString, (char*)dataBuffer);
									break;
								case('XXXT'):
								case('XXT'):

									if (whereIsIt = strstr(id3Buffer, "Engineer"))
										metadata[8] = whereIsIt + strlen("Engineer") + 1;   //basically just advanced ahead of the string and 1 space
									else if (whereIsIt = strstr(id3Buffer, "Encoded by"))
										metadata[META_SOFTWARE] = whereIsIt + strlen("Encoded by") + 1;
									else if (whereIsIt = strstr(id3Buffer, "Software"))
										metadata[META_SOFTWARE] = whereIsIt + strlen("Software") + 1;

								default:
									metadata[j] = (char*)dataBuffer;

									while (metadata[j][0] == ' ')
										metadata[j]++;
									while (metadata[j][strlen(metadata[j])-1] == ' ')
										metadata[j][strlen(metadata[j])-1] = 0;

									
								}
								
							}
							if ((id3Tag == flip('TDAT') || id3Tag == flip('TDA')) && yearString)
							{

								*strrchr(yearString, ' ') = 0;
								strcat(yearString, "-");
								strncat(yearString, id3Buffer + 1, 2);

								strcat(yearString, "-");
								strncat(yearString, id3Buffer + 3, 2);
								metadata[META_CREATIONDATE] = yearString;
								if(metadata[META_CREATIONDATE])
									while (metadata[META_CREATIONDATE][0] == ' ')
										metadata[META_CREATIONDATE]++;
							
								break;
							}

								}
							}
							

								
							
						}
				if (*intSeeker == flip('APIC'))
				{
					//fun emmbed dded foto
					for (int k = 0; k < 64; k++)
					{
						if ( *(int*)(&buffer[i]+k) ==  0xE0FFD8FF && *(int*)(&buffer[i]+k+6) == 'FIFJ') //jfif start of image num
						{
							printf("Embedded JFIF found, extracting\n");
							void* startOfJFIF = (char*)(intSeeker)+k;
							char* jpgFileName = (char*)calloc(_MAX_PATH, 1);
							strcpy(jpgFileName, filePathLowered);
							char* fileExtensionPtr = 0;
							if (!(fileExtensionPtr = strstr(jpgFileName, ".wav")))
								fileExtensionPtr = strstr(jpgFileName, ".mp3");
							*fileExtensionPtr = 0;
							strcat(jpgFileName, ".jpg");
							FILE* jpgFile = fopen(jpgFileName, "wb");
							fwrite(startOfJFIF, fileLength - i, 1, jpgFile);
							fclose(jpgFile);
							
						}
					}

				}
				
			}
			
		}
		
		bool fileDontExists = false;
		char* csvPath = argv[2];
		if (!fopen(csvPath, "r")) //the csv does not exist
		{
			fileDontExists = true;
		}
		FILE* csvFile = fopen(csvPath, "a");
		if (fileDontExists)
			fprintf(csvFile, "File Path,Track title,Album,Artist,Creation Date,Track Number,Comment,Keywords,Software,Engineer,Technician,Genre,Copyright,Subject,Source\n");
		printf("File Path,Track title,Album,Artist,Creation Date,Track Number,Comment,Keywords,Software,Engineer,Technician,Genre,Copyright,Subject,Source\n");

		char* final = (char*)calloc(1024,1);
		strcat(final, "\"");
		strcat(final, argv[1]);
		strcat(final, "\"");
		strcat(final, ",");
		for (int i = 0; i < metaNum; i++)
		{
			if (metadata[i])
			{
				strcat(final, "\"");
				
				strcat(final, metadata[i]);
				strcat(final, "\"");
				
			}
			else
				strcat(final, " ");
			strcat(final, ",");
		}
		strcat(final, "\n");
		printf("%s", final);
		fprintf(csvFile, final);
		return 0;
	}
	return -1;
}
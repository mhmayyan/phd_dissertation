
/**
By Mohammad Mayyan, August 1st, 2022
*/


#include "smartDiningTable.h"

inline static int compareDesending (const void * a, const void * b)
{
  return ( *(int*)b - *(int*)a );
}

/*****************************************************/
static bool loadGroundTruthValues(char *fileName, const int64_t length)
{

	// retrievedGroundTruth
	if( access( fileName , F_OK ) == 0 )
	{

		if((fpt = fopen(fileName,"r")) == NULL)
    {
        printf("Oops!!\n\tUnable to open file \"%s\"\n",fileName);
        return false;
    }
		/**************************************************************/
		/*make sure we have the number of line */
		int64_t localNumberOfLines = 0;
		int inChar;
		while (1)
	  {
			inChar = getc(fpt);
			if (inChar == '\n') {
				localNumberOfLines++;
			}
			if (inChar == EOF) {
				break;
			}
	  }
		/* Return if there were no frames  */
		if (localNumberOfLines == 0) {
			fclose(fpt);
			printf("Error while opening file. There were no frames!\n");
			return 0;
		}
		rewind(fpt);
		if (localNumberOfLines != length) {
			fclose(fpt);
			printf("ERROR localNumberOfLines != length \n");
			return false;
		}
		/**************************************************************/
		int **temp_retrievedGroundTruth = (int**)malloc(sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);
		if(temp_retrievedGroundTruth == NULL) {printf("ERROR temp_retrievedGroundTruth malloc \n"); fflush(stdout); exit(-1);}
		for(int ii=0; ii<NUMBER_OF_OBJECTS_TO_TRACK; ii++)
		{
			temp_retrievedGroundTruth[ii] = (int*)malloc(sizeof(int)*length);
			if(temp_retrievedGroundTruth[ii] == NULL)  {printf("ERROR temp_retrievedGroundTruth[%d] malloc \n", ii); fflush(stdout); exit(-1);}
			memset(temp_retrievedGroundTruth[ii], 0, sizeof(int)*length);
		}

		// retrieve GroundTruth values


		int reading;
		int64_t ii=0, TotalData=0;
		bool goodReading = true;

		while (goodReading)
		{
		  ii=fscanf(fpt,"%d",&reading);
		  if (ii != 1)
			{
				break;
			}
			temp_retrievedGroundTruth[0][TotalData]=reading; /*plate*/
		  for (ii=1; ii<NUMBER_OF_OBJECTS_TO_TRACK; ii++)
			{
				if(fscanf(fpt,"%d",&reading) != 1)
				{
					goodReading = false;
					break;
				}
				temp_retrievedGroundTruth[ii][TotalData]=reading;
			}

			if (!goodReading) {
				break;
			}

		  TotalData++;

			/*limit the while loop*/
			if (TotalData > length) {
				break;
			}
		} // while (goodReading)


		fclose(fpt);
		if (length != TotalData) {
			printf("Oops!!\n\t GroungTruth data length != TotalData \n");
			printf("length: %ld \t TotalData: %ld \n", length, TotalData);
			fflush(stdout);
			return false;
		} else //if (length == TotalData)
		{
			retrievedGroundTruth = (int**)malloc(sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);
			if(retrievedGroundTruth == NULL) {printf("ERROR retrievedGroundTruth malloc \n"); fflush(stdout); exit(-1);}
			for(int ii=0; ii<NUMBER_OF_OBJECTS_TO_TRACK; ii++)
			{
				retrievedGroundTruth[ii] = (int*)malloc(sizeof(int)*length);
				if(retrievedGroundTruth[ii] == NULL)  {printf("ERROR retrievedGroundTruth[%d] malloc \n", ii); fflush(stdout); exit(-1);}
				memset(retrievedGroundTruth[ii], 0, sizeof(int)*length);
				memcpy(retrievedGroundTruth[ii], temp_retrievedGroundTruth[ii], sizeof(int)*length);
				free(temp_retrievedGroundTruth[ii]);
			}
			free(temp_retrievedGroundTruth);

			printf("GroungTruth data were loaded successfully! \n");

		}

	}
	else
	{
		printf("Oops!!\n\tUnable to open file \"%s\"\n",fileName);
		return false;
	}

	return true;
}






static void loadHardCodedCalibrationValues(void)
{
	const double hardCodedCalibrationValues[NUMBER_OF_GRID_SENSELS] =
	{
		13103.22,		13308.3,		13057.73,		13239.08,		12649.01,		1.00000e9,		13212.9,		13655.61,		13440.11,		12872.35,		12073.79,
		12288.01,		12874.11,		13110.42,		12678.02,		13933.6,		13929.1,		13638.89,		13146.79,		13412.69,		13618.06,		13878.39,
		13923.85,		12422.7,		12829.37,		14063.71,		13083.84,		13691.1,		13700.83,		12512.93,		12723.79,		12731.15,		11910.74,
		13503.29,		12621.22,		13422.22,		13679.76,		13397.08,		13958.98,		12929.76,		12312.33,		12747.52,		13016.16,		11033.05,
		12795.74,		12588.12,		13589.63,		13815.35,		13173.77,		14501.38,		13393.1,		12386.93,		13230.72,		13206.72,		12716.08,
		13068.8,		13103.62,		13966.8,		12632.99,		12589.47,		12860.9,		14141.21,		13003.84,		13094.94,		11272.37,		10976.19,
		13285.79,		13290.29,		12558.92,		12813.06,		13220.93,		12728.92,		13301.9,		12221.25,		12413.56,		10508.07,		11878.85

		// 13104.820,	13184.850,	13059.830,	13240.750,	12651.210,	1.00000e9,	13212.450,	13649.030,	13433.630,	12873.660,	12046.780,
		// 12289.570,	12876.430,	13109.510,	12681.440,	13941.440,	13935.180,	13641.940,	13156.560,	13417.020,	13618.080,	13876.070,
		// 13929.630,	12420.600,	12420.600,	14065.600,	13089.170,	13694.160,	13706.520,	12516.030,	12724.890,	12735.550,	11889.330,
		// 13504.350,	12622.450,	13425.330,	13680.690,	13399.060,	13962.900,	12929.580,	12317.880,	12749.900,	13018.970,	10980.720,
		// 12799.800,	12593.290,	13595.720,	13821.760,	13177.320,	14509.460,	13395.870,	12393.710,	13214.140,	13210.430,	12699.920,
		// 13072.220,	13109.160,	13971.150,	12637.220,	12589.490,	12862.880,	14143.710,	13012.540,	13096.280,	11262.930,	10932.120,
		// 13287.330,	13296.070,	12562.720,	12818.370,	13223.830,	12730.800,	13304.210,	12220.200,	12415.950,	10494.750,	10494.750
	};
	// printf("Loading default hard-coded calibration values\n" );
	memcpy(calibrationArray, hardCodedCalibrationValues, NUMBER_OF_GRID_SENSELS*sizeof(double));
	// saveCalibrationValues(HARD_CODED_CALIBRATION_VALUES_FILE_NAME);
	// strcpy(lastLoadedCalibrationFileName, HARD_CODED_CALIBRATION_VALUES_FILE_NAME);
	// printf("Default calibration values were saved in the file: %s\n", lastLoadedCalibrationFileName);
}



static unsigned int assureSequenceOfFrames(int **dataBuff, int64_t numberOfFrames)
{
	int frameIndex, prevFrameIndex;
	prevFrameIndex = dataBuff[0][NUMBER_OF_GRID_SENSELS+1];
	for (int ind = 1; ind < numberOfFrames; ind++) {
		frameIndex = dataBuff[ind][NUMBER_OF_GRID_SENSELS+1];
		if (frameIndex != (prevFrameIndex+1)) {
			return frameIndex;
		}
		prevFrameIndex = frameIndex;
	}
	return 0;
}


static char *removeExtension(char const *myStr) {
    char *retStr;
    char *lastExt;
    if (myStr == NULL) return NULL;
    if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;
    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, '.');
    if (lastExt != NULL)
        *lastExt = '\0';
    return retStr;
}

void loadFiles(char const **files)
{
	loadHardCodedCalibrationValues();
	char *fullPathWithoutExtension = removeExtension(files[1]);
	if(fullPathWithoutExtension == NULL)
	{
		printf("Error removing file extention \n");
		exit(-1);
	}

	strncpy(groungTruthFullPathFileName, fullPathWithoutExtension, sizeof(groungTruthFullPathFileName));
	strncat(groungTruthFullPathFileName, GROUND_TRUTH_FILE_EXTENSION, sizeof(GROUND_TRUTH_FILE_EXTENSION)+2);
	printf("Ground truth full name: %s\n", groungTruthFullPathFileName); fflush(stdout);

	if((fpt = fopen(files[1],"r")) == NULL)
	{
		printf("Error while opening file!\n");
		exit(-1);
	}

	numberOfDataFrames = 0;
	int inChar;
	while (1)
  {
		inChar = getc(fpt);
		if (inChar == '\n') {
			numberOfDataFrames++;
		}
		if (inChar == EOF) {
			break;
		}
  }
	/* Return if there were no frames  */
	if (numberOfDataFrames == 0) {
		fclose(fpt);
		printf("Error while opening file. There were no frames!\n");
		fflush(stdout);
		fclose(fpt);
		exit(-1);
	}
	rewind(fpt);


	DataFrames = (int**)malloc(sizeof(int*)*numberOfDataFrames);

	if (DataFrames != NULL) {
		//

		memset(DataFrames, 0, sizeof(int*)*numberOfDataFrames);
		for(int64_t i=0; i<numberOfDataFrames; i++)
	  {
	     DataFrames[i] = (int*)malloc(sizeof(int)*(NUMBER_OF_GRID_SENSELS+2));

			 if (DataFrames[i] == NULL)
			 {
				 printf("Error malloc %ld !\n", i);
				 fflush(stdout);
				 fclose(fpt);
				 exit(-1);
			 }
			 memset(DataFrames[i], 0, sizeof(int)*(NUMBER_OF_GRID_SENSELS+2));
	  }
	}
	else
	{
		printf("Error malloc for DataFrames !\n");
		fflush(stdout);
		fclose(fpt);
		exit(-1);
	}



	int reading;
	rewind(fpt);
	int i=0, TotalData=0;
	bool goodReading = true;
	while (goodReading)
	{
	  i=fscanf(fpt,"%d",&reading);
	  if (i != 1)
		{
			break;
		}
		DataFrames[TotalData][0]=reading;
	  for (i=1; i<(NUMBER_OF_GRID_SENSELS+2); i++)
		{
			if(fscanf(fpt,"%d",&DataFrames[TotalData][i]) != 1)
			{
				goodReading = false;
				break;
			}
		}
		if (!goodReading) {
			break;
		}
	  TotalData++;
	}


	if (numberOfDataFrames != TotalData) {
		printf("Error numberOfDataFrames != TotalData  !\n");
		fflush(stdout);
		fclose(fpt);
		exit(-1);
	}

	printf("numberOfDataFrames == %ld !\n", numberOfDataFrames); fflush(stdout);

	/*** Assure correct sequence of frames ***/
	unsigned int missingFrameAt;
	if ((missingFrameAt = assureSequenceOfFrames(DataFrames, numberOfDataFrames)) > 0) {
		printf("Error: there is a missing frame in file! See frame %d\n", missingFrameAt);
		fflush(stdout);
		fclose(fpt);
		exit(-1);
	}

	fclose(fpt);

	/*make sure we have enough data*/
	if(numberOfDataFrames<TARE_CALIBRATION_ARRAY_LENGTH)
	{
		printf("Error: must have at least 51 frames with noting on the table surface!\n");
		fflush(stdout);
		exit(-1);
	}
	/*copy data for median calculation*/
	for (int frm = 0; frm < TARE_CALIBRATION_ARRAY_LENGTH; frm++) {
		for (int snsl = 0; snsl < NUMBER_OF_GRID_SENSELS; snsl++) {
			readingsArrayMatrix[snsl][frm] = DataFrames[frm][snsl];
		}
	}
	/*compute tareArray*/
	for (int snsl = 0; snsl < NUMBER_OF_GRID_SENSELS; snsl++) {
		qsort (readingsArrayMatrix[snsl], TARE_CALIBRATION_ARRAY_LENGTH, sizeof(int), compareDesending);
		tareArray[snsl] = (readingsArrayMatrix[snsl][TARE_CALIBRATION_ARRAY_LENGTH/2]);
	}


	/*compute grams for DataFramesInGrams */
	DataFramesInGrams = (double**)malloc(sizeof(double*)*numberOfDataFrames);
	if (DataFramesInGrams == NULL)
	{
		printf("Error: malloc for DataFramesInGrams!\n");
		fflush(stdout);
		exit(-1);
	}
	memset(DataFramesInGrams, 0, sizeof(double*)*numberOfDataFrames);
	for(int64_t ii=0; ii<numberOfDataFrames; ii++)
	{
		 DataFramesInGrams[ii] = (double*)malloc(sizeof(double)*(NUMBER_OF_GRID_SENSELS));
		 /*** if failed to malloc then free all  ***/
		 if (DataFramesInGrams[ii] == NULL)
		 {
			 printf("Error malloc DataFramesInGrams %ld !\n", ii);
			 fflush(stdout);
			 exit(-1);
		 }
	 }
	for (int64_t frm = 0; frm < numberOfDataFrames; frm++)
	{
		for (int ind = 0; ind < NUMBER_OF_GRID_SENSELS; ind++) {
			DataFramesInGrams[frm][ind] = ((double)DataFrames[frm][ind] - tareArray[ind])/calibrationArray[ind];
		}
	}

	/*************************************************************************/
	/*************************************************************************/
	/*************************************************************************/
	/** retreive GroundTruth*/
	groundTruthLoaded = loadGroundTruthValues(groungTruthFullPathFileName, numberOfDataFrames);




} // loadFiles

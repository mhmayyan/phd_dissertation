
/**
By Mohammad Mayyan, August 1st, 2022
*/


#include "smartDiningTable.h"




/****************************************************/
/**
 * computes the standard deviation
 */
double getStandardDeviation(const double *value, const int size);
inline double getStandardDeviation(const double *value, const int size)
{
	int ii;
	double //value [MAXSIZE],
	deviation,sum,sumsqr,mean,variance,stddeviation;
	sum = sumsqr = 0 ;
	for (ii=0; ii< size ; ii++) {
	  sum += value[ii];
	}
	mean = sum/(double)size;
	for (ii = 0 ; ii< size; ii++) {
	  deviation = value[ii] - mean;
	  sumsqr += deviation * deviation;
	}
	variance = sumsqr/(double)size ;
	stddeviation = sqrt(variance) ;
	return stddeviation;
}
/*******************************************/
/**
 * checks if all measurements are stable for the past
 * DETECTION_ALGORITHM_WINDOW_SIZE including the index frm
 */
bool measurementsAreStable(
 	const double *_moving_STD,
 	const int frm,
 	const int waitForStability
);
inline bool measurementsAreStable(
	const double *_moving_STD,
	const int frm,
	const int waitForStability
)
{
	bool measurementsAreStable = true;
	for (int ii = frm+1-waitForStability/*DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY*/; ii < frm+1; ii++) {
		if (_moving_STD[ii]>STD_THRESHOLD_FOR_STABILITY) {
			measurementsAreStable = false;
			break;
		}
	}
	return measurementsAreStable;
}
/****************************************************/
int doubleCmpFunc (const void * a, const void * b);
inline int doubleCmpFunc (const void * a, const void * b)
{
   return (((*(double *)a) - (*(double *)b))>0?1:-1);
}
/****************************************************/
bool getLSW(
	double **eating_Profile,
	double **moving_STD,
	int indx, // start from this frame and search backward
	const int object, /*0:plate, 1:cup, 2:bowl*/
	const int waitForStability,
	double *weightBeforeChange
);
inline bool getLSW(
	double **eating_Profile,
	double **moving_STD,
	int indx, // start from this frame and search backward
	const int object, /*0:plate, 1:cup, 2:bowl*/
	const int waitForStability,
	double *weightBeforeChange
)
{
	// int indx = frameToStartAt;
	//
	// bool retVal = false;

	// bool weightBeforeChangeCorrect = false;
	// int indx = thereIsChange[object]; /* search backword for stable window starting from thereIsChange */
	while(indx>=DETECTION_ALGORITHM_WINDOW_SIZE) /*# to not access array at negative address*/
	{
		if(
			measurementsAreStable(moving_STD[object], indx, waitForStability)
			// /*make sure object is present on the grid and it does not measure zero*/
			// &&
			// (	/*there must be weight on the object*/
			// 	eating_Profile[object][indx]>1.0f /*maybe I need to use different weight for each object*/
			// 	// || /*except for the first placement of objects*/
			// 	// (firstDetectionOccurred[object]==0 /*never detected*/
			// 	// 	&& moving_STD[object][indx]==0 /*std is zero when the object was not on the surface */
			// 	// )
			// )
		)
		{
			double values[DETECTION_ALGORITHM_WINDOW_SIZE];
			memcpy(values, &(eating_Profile[object][indx+1-DETECTION_ALGORITHM_WINDOW_SIZE]), DETECTION_ALGORITHM_WINDOW_SIZE*sizeof(double));
			qsort(values, DETECTION_ALGORITHM_WINDOW_SIZE, sizeof(double), doubleCmpFunc);
			*weightBeforeChange = values[DETECTION_ALGORITHM_WINDOW_SIZE/2];
			// weightBeforeChange = np.median(plateWeight[indx-DETECTION_ALGORITHM_WINDOW_SIZE:indx])
			// weightBeforeChangeCorrect = true;
			return true; // break;

		}
		indx--;
	}

	return false;
}

inline static void detectionAlgorithm(
	int **region_Indices,
	double **eating_Profile,
	const int object, /*0:plate, 1:cup, 2:bowl*/
	const int64_t frm,
	double **detected_Weights,
	double **moving_STD,
	int **detection_Spot,
	int *thereIsChange,
	int **last_Location,	/*last stable location*/
	int *firstDetectionOccurred,
	uint32_t *highSTD_Duration,
	int64_t *massBitePickedUpAt,
	double *utensilAdded
)
{
	(void) firstDetectionOccurred;
	(void) highSTD_Duration;

	if(frm<DETECTION_ALGORITHM_WINDOW_SIZE+1) return;

	double currentSTD = getStandardDeviation(&(eating_Profile[object][frm+1-DETECTION_ALGORITHM_WINDOW_SIZE]), DETECTION_ALGORITHM_WINDOW_SIZE); /*std(plateWeight[frm+1-DETECTION_ALGORITHM_WINDOW_SIZE:frm+1])*/
	moving_STD[object][frm] = currentSTD;

	double lastStableWeight=0.0f;

	/**************************************/
	/**************************************/
	/**************************************/
	if (currentSTD > STD_THRESHOLD_FOR_CHANGE)
	{
		thereIsChange[object] = frm;
	}

	/**************************************/
	/**************************************/
	/**************************************/
	if (thereIsChange[object] && currentSTD < STD_THRESHOLD_FOR_STABILITY)
	{

		if(
				utensilAdded[object] > 0
				&& measurementsAreStable(moving_STD[object], frm, DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY_UTENSIL)
				// && objectStillOnSurface[object]
			)
		{

			double weightAfterChange=0;
			for (int frmIndx = 0; frmIndx < DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY_UTENSIL; frmIndx++) {
				weightAfterChange += eating_Profile[object][frm-frmIndx];
			}
			weightAfterChange /= (double)DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY_UTENSIL;
			/*look backward for last stable weight (LSW)*/
			if(
					getLSW(
						eating_Profile,
						moving_STD,
						thereIsChange[object], // start from this frame and search backward
						object,
						DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY,
						&lastStableWeight //&(lastStableWeight[object])
					)
				)
			{
				/**/

				double weightDiff = weightAfterChange - lastStableWeight;//[object];

				if (weightDiff<(-1*MIN_WEIGHT_TO_DETECT)) {

					if
						(
							fabs(weightDiff) >= (utensilAdded[object]-WEIGHT_OF_UTENSIL_MARGIN) //(WEIGHT_OF_UTENSIL-WEIGHT_OF_UTENSIL_MARGIN) /*weight of utensil is in range*/
							&&
							fabs(weightDiff) <= (utensilAdded[object]+WEIGHT_OF_UTENSIL_MARGIN) //(WEIGHT_OF_UTENSIL+WEIGHT_OF_UTENSIL_MARGIN) /*weight of utensil is in range*/
						// && highSTD_Duration[object]<20 //highSTD_DurationBckup < 10 /*picking up the utensile is usually fast*/
						)
					{
						utensilAdded[object] = 0; /*reset the utensile flag as it is now removed*/
						thereIsChange[object] = 0;
						return;
					}
				}
			}
		}

		/**************************************/
		/**************************************/
		/**************************************/
		if (
				 measurementsAreStable(moving_STD[object], frm, DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY)
				// && objectStillOnSurface[object]
			)
		{

			double values[DETECTION_ALGORITHM_WINDOW_SIZE];
			memcpy(values, &(eating_Profile[object][frm+1-DETECTION_ALGORITHM_WINDOW_SIZE]), DETECTION_ALGORITHM_WINDOW_SIZE*sizeof(double));
			qsort(values, DETECTION_ALGORITHM_WINDOW_SIZE, sizeof(double), doubleCmpFunc);
			double weightAfterChange = values[DETECTION_ALGORITHM_WINDOW_SIZE/2];

			/*look backward for last stable weight (LSW)*/
			if(
					getLSW(
						eating_Profile,
						moving_STD,
						thereIsChange[object], // start from this frame and search backward
						object,
						DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY,
						&lastStableWeight //&(lastStableWeight[object])
					)
				)
			{
				thereIsChange[object] = 0;
				/**/
				double weightDiff = weightAfterChange - lastStableWeight;//[object];

				/*skip if weightDiff is just noise*/
				if (weightDiff > (-1*MIN_WEIGHT_TO_DETECT) && weightDiff < MIN_WEIGHT_TO_DETECT)
				{
					// thereIsChange[object] = 0;
					return;
				}

				if(weightDiff>0) /*weight added*/
				{
					/*mass bite returned*/
					if(	weightDiff >= 40)
					{
						if ( massBitePickedUpAt[object])
						{
							double biteConsumed = detected_Weights[object][massBitePickedUpAt[object]] + weightDiff;

							if(biteConsumed < (-1*MIN_WEIGHT_TO_DETECT))
							{
								detected_Weights[object][massBitePickedUpAt[object]] = biteConsumed;
								detection_Spot[object][massBitePickedUpAt[object]] = 2; //mass bite
								massBitePickedUpAt[object]=0;
							}
							else /*if returned mass bite was bigger than that picked up*/
							{
								massBitePickedUpAt[object]=0;
							}
						}
					}
					/*if the weight difference is equal to the weight of utensil +/- 1 gram then set the appropriate flag*/
					else if
					(
						weightDiff >= 1.0f //(WEIGHT_OF_UTENSIL-WEIGHT_OF_UTENSIL_MARGIN)
						&&
						weightDiff <= 6.0f //(WEIGHT_OF_UTENSIL+WEIGHT_OF_UTENSIL_MARGIN)
					)
					{
						utensilAdded[object]=weightDiff;
					}
				} //if(weightDiff>0) /*weight added*/
				else /*picked up*/
				{
					detected_Weights[object][frm] = weightDiff;
					utensilAdded[object] = 0;

					/*mass bite picked up*/
					if (weightDiff <= -40)// && !massBitePickedUpAt)
					{
						massBitePickedUpAt[object] = frm; /*save where this bite was picked up*/
						detection_Spot[object][frm] = 2; //mass bite
						//// lastStableWeightBeforeMassBitePickedUp[object] = lastStableWeight[object];
					}
					/*consider small weight reductions if placement of utensil was not detected */
					else //if (!utensilAdded[object])
					{
						detection_Spot[object][frm] = 1;
					}
				} //else /*picked up*/
			} // if(getLSW())

			// thereIsChange[object] = 0;

		} //if (measurementsAreStable() && objectStillOnSurface)

	} // if(thereIsChange[object] && currentSTD < STD_THRESHOLD_FOR_STABILITY)


	/*reset the mass bite flag if times out*/
	if
		(
		massBitePickedUpAt[object]
		&& (
			frm-massBitePickedUpAt[object]> 10*60*10/*10 minutes*/
			|| frm==0
			)
		)
	{
		massBitePickedUpAt[object] = 0; //
	}

	/*if it's been quiet for so long without detection then reset thereIsChange */
	if (measurementsAreStable(moving_STD[object], frm, DETECTION_ALGORITHM_WINDOW_SIZE_FOR_STABILITY) && thereIsChange[object]>0 && (frm>(thereIsChange[object]+DETECTION_ALGORITHM_WINDOW_SIZE+3)))
	{
		thereIsChange[object] = 0;
		// return;
	}

	/*save the last location when currentSTD<0.01*/
	memcpy(last_Location[object], region_Indices[object], NUMBER_OF_GRID_SENSELS*sizeof(int));



}












void processSegmentationAndBiteDetection(void)
{
	/*************************************************************************/
	/********* detection events ******************/
	detectedWeights = (double**)malloc(sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if (detectedWeights == NULL)
	{
		printf("Error malloc detectedWeights !\n");
		fflush(stdout);
		exit(-1);
	}
	movingSTD = (double**)malloc(sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if (movingSTD == NULL)
	{
		printf("Error malloc movingSTD !\n");
		fflush(stdout);
		exit(-1);
	}
	detectionSpot = (int**)malloc(sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if (detectionSpot == NULL)
	{
		printf("Error malloc detectionSpot !\n");
		fflush(stdout);
		exit(-1);
	}

	memset(detectedWeights, 0, sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	memset(movingSTD, 0, sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	memset(detectionSpot, 0, sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);

	for(int ii=0; ii<NUMBER_OF_OBJECTS_TO_TRACK; ii++)
	{
		 detectedWeights[ii] = (double*)malloc(sizeof(double)*numberOfDataFrames);
		 if (detectedWeights[ii] == NULL)
		 {
			 printf("Error malloc detectedWeights %d !\n", ii);
			 fflush(stdout);
			 exit(-1);
		 }
		 movingSTD[ii] = (double*)malloc(sizeof(double)*numberOfDataFrames);
		 if (movingSTD[ii] == NULL)
		 {
			 printf("Error malloc movingSTD %d !\n", ii);
			 fflush(stdout);
			 exit(-1);
		 }
		 detectionSpot[ii] = (int*)malloc(sizeof(int)*numberOfDataFrames);
		 if (detectionSpot[ii] == NULL)
		 {
			 printf("Error malloc detectionSpot %d !\n", ii);
			 fflush(stdout);
			 exit(-1);
		 }

		 memset(detectedWeights[ii], 0, sizeof(double)*numberOfDataFrames);
		 memset(movingSTD[ii], 0, sizeof(double)*numberOfDataFrames);
		 memset(detectionSpot[ii], 0, sizeof(int)*numberOfDataFrames);


	}




	/********* eating profile ******************/
	eatingProfile = (double**)malloc(sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(eatingProfile == NULL) {printf("ERROR eatingProfile malloc \n"); fflush(stdout); exit(-1);}
	memset(eatingProfile, 0, sizeof(double*)*NUMBER_OF_OBJECTS_TO_TRACK);
	for(int i=0; i<NUMBER_OF_OBJECTS_TO_TRACK; i++)
	{
		 eatingProfile[i] = (double*)malloc(sizeof(double)*numberOfDataFrames);
		 if(eatingProfile[i] == NULL) {printf("ERROR eatingProfile[%d] malloc \n", i); fflush(stdout); exit(-1);}
		 memset(eatingProfile[i], 0, sizeof(double)*numberOfDataFrames);
	}



	int *thereIsChange = (int*)malloc(sizeof(int)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(thereIsChange == NULL) {printf("ERROR thereIsChange malloc \n"); fflush(stdout); exit(-1);}
	memset(thereIsChange, 0, NUMBER_OF_OBJECTS_TO_TRACK*sizeof(int));
	int **lastLocation = (int**)malloc(sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(lastLocation == NULL) {printf("ERROR lastLocation malloc \n"); fflush(stdout); exit(-1);}
	for (int ii = 0; ii < NUMBER_OF_OBJECTS_TO_TRACK; ii++) {
		lastLocation[ii] = (int*)malloc(sizeof(int)*NUMBER_OF_GRID_SENSELS);
		if(lastLocation[ii] == NULL) {printf("ERROR lastLocation[ii] malloc \n"); fflush(stdout); exit(-1);}
		memset(lastLocation[ii], 0, NUMBER_OF_GRID_SENSELS*sizeof(int));
	}





	int *firstDetectionOccurred = (int*)malloc(sizeof(int)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(firstDetectionOccurred == NULL) {printf("ERROR firstDetectionOccurred malloc \n"); fflush(stdout); exit(-1);}
	memset(firstDetectionOccurred, 0, NUMBER_OF_OBJECTS_TO_TRACK*sizeof(int));


	uint32_t *highSTD_Duration = (uint32_t*)malloc(sizeof(uint32_t)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(highSTD_Duration == NULL) {printf("ERROR highSTD_Duration malloc \n"); fflush(stdout); exit(-1);}
	memset(highSTD_Duration, 0, NUMBER_OF_OBJECTS_TO_TRACK*sizeof(uint32_t));

	int64_t *massBitePickedUpAt = (int64_t*)malloc(sizeof(int64_t)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(massBitePickedUpAt == NULL) {printf("ERROR massBitePickedUpAt malloc \n"); fflush(stdout); exit(-1);}
	memset(massBitePickedUpAt, 0, NUMBER_OF_OBJECTS_TO_TRACK*sizeof(int64_t));

	double *utensilAdded = (double*)malloc(sizeof(double)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(utensilAdded == NULL) {printf("ERROR utensilAdded malloc \n"); fflush(stdout); exit(-1);}
	memset(utensilAdded, 0, NUMBER_OF_OBJECTS_TO_TRACK*sizeof(double));


	regionIndices = (int**)malloc(sizeof(int*)*NUMBER_OF_OBJECTS_TO_TRACK);
	if(regionIndices == NULL) {printf("ERROR regionIndices malloc \n"); fflush(stdout); exit(-1);}
	for (int ii = 0; ii < NUMBER_OF_OBJECTS_TO_TRACK; ii++) {
		regionIndices[ii] = (int*)malloc(sizeof(int)*NUMBER_OF_GRID_SENSELS);
		if(regionIndices[ii] == NULL) {printf("ERROR regionIndices[ii] malloc \n"); fflush(stdout); exit(-1);}
	}

	/************************************************************************/
	/****** My Segmentation using moving circle object over the grid ********/
	senselOrigion = (int***)malloc(NUMBER_OF_GRID_ROWS*sizeof(int**));
	if (senselOrigion != NULL) {
		for (int row = 0; row < NUMBER_OF_GRID_ROWS; row++) {
			senselOrigion[row] = (int**)malloc(NUMBER_OF_GRID_COLUMNS*sizeof(int*));
			if (senselOrigion[row] != NULL) {
				for (int col = 0; col < NUMBER_OF_GRID_COLUMNS; col++) {
					senselOrigion[row][col] = (int*)malloc(2*sizeof(int));
					if (senselOrigion[row][col] != NULL) {
						senselOrigion[row][col][0] = col*SENSEL_SIDE_LENGTH_MM+col*SPACING_BETWEEN_ADJACENT_TILES_MM; // x
						senselOrigion[row][col][1] = row*SENSEL_SIDE_LENGTH_MM+row*SPACING_BETWEEN_ADJACENT_TILES_MM; // y
					}  else {printf("ERROR malloc senselOrigion[row][col]\n"); exit(0);}
				}
			}  else {printf("ERROR malloc senselOrigion[row]\n"); exit(0);}
		}
	} else {printf("ERROR malloc senselOrigion\n"); exit(0);}



	// /****** segmentation *****/
	printf("Segmentation progress:    "); fflush(stdout);
	int progress=0;

	for(int64_t frm=0; frm<numberOfDataFrames; frm++)
	{

		/** show progress on the consol**/
		if(frm%100 == 0)
		{
			progress = frm*100.0f/numberOfDataFrames;
			if (progress<10) {
				printf("\b\b\b0%d%%", progress);
			} else
			{
				printf("\b\b\b%d%%", progress);
			}
			fflush(stdout);

		}





		SegmentRegions(DataFramesInGrams[frm]);
		eatingProfile[PLATE_SEGMENT_LABEL-1][frm] = RegionWeight[PLATE_SEGMENT_LABEL];
		eatingProfile[CUP_SEGMENT_LABEL-1][frm] = RegionWeight[CUP_SEGMENT_LABEL];
		eatingProfile[BOWL_SEGMENT_LABEL-1][frm] = RegionWeight[BOWL_SEGMENT_LABEL];



		for (int object = 0; object < 3; object++)
		{
			detectionAlgorithm(
				regionIndices,
				eatingProfile,		/*can be saved*/
				object,
				frm,
				detectedWeights,		/*can be saved*/
				movingSTD,		/*can be saved*/
				detectionSpot,		/*can be saved*/
				thereIsChange,
				lastLocation,
				firstDetectionOccurred,
				highSTD_Duration,
				massBitePickedUpAt,
				utensilAdded
			);


		}
	} // for(int frm=0; frm<numberOfDataFrames; frm++)

	printf("\n\n");


	for (int ii = 0; ii < NUMBER_OF_OBJECTS_TO_TRACK; ii++) {
		int numOfBites = 0;
		for(int frm=0; frm<numberOfDataFrames; frm++) if(detectedWeights[ii][frm]) numOfBites++;
		printf("Detections [%d] = %d \n", ii, numOfBites);
	}
	printf("\n\n");

}

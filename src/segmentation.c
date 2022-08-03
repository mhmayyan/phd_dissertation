

/**
By Mohammad Mayyan, August 1st, 2022
*/

#include "smartDiningTable.h"






inline static void postProcessRegionGrowing(double *imageOfWeightsInGrams, int *SenselIndices, int *countOfPixelsPainted, unsigned char label)
{
	countOfPixelsInRegions[label] = *countOfPixelsPainted;

	memset(regionIndices[label-1], 0, NUMBER_OF_GRID_SENSELS*sizeof(int)); /*delete previous data*/
	for (int i=0; i<(*countOfPixelsPainted); i++)
	{
		RegionWeight[label]+=imageOfWeightsInGrams[SenselIndices[i]];
		Regions[SenselIndices[i]] = label;
		regionIndices[label-1][i] = SenselIndices[i];
	}
}


/* This function groups sensels together into regions.  Each region is assumed to be a container (e.g. plate, cup). */

inline void SegmentRegions(double *imageOfWeightsInGrams)
{
	/*to make sure we keep only new data for every frame*/
	memset(RegionWeight, 0, NUMBER_OF_GRID_SENSELS*sizeof(double));

	int			  SenselIndices[NUMBER_OF_GRID_SENSELS],countOfPixelsPainted;


	memset(Regions, 0, sizeof(unsigned char)*NUMBER_OF_GRID_SENSELS);


	/********************* find the region under the plate *********************/
	if(
			getRegionUnderObject(
				imageOfWeightsInGrams,			/* image data */
			  Regions,  /* segmentation labels */
				0,      /* cup is on the right. Bowl is on the left */
				NUMBER_OF_GRID_COLUMNS,       /* cup is on the right. Bowl is on the left */
			  PLATE_DIAMETER,     /* object diameter in mm */
			  SenselIndices,           /* output:  indices of pixels painted */
			  &countOfPixelsPainted,             /* output:  count of pixels painted */
				10,					/* step size (mm) at which the center of plate is moved through iterations */
				PLATE_SEGMENT_LABEL
			)
	)
	{
		postProcessRegionGrowing(imageOfWeightsInGrams, SenselIndices, &countOfPixelsPainted, PLATE_SEGMENT_LABEL);
	}

	/********************* find the region under the cup *********************/
	if(
			getRegionUnderObject(
				imageOfWeightsInGrams,			/* image data */
				Regions,  /* segmentation labels */
				6,      /* cup is on the right. Bowl is on the left */
				NUMBER_OF_GRID_COLUMNS,       /* cup is on the right. Bowl is on the left */
				CUP_DIAMETER,     /* object diameter in mm */
				SenselIndices,           /* output:  indices of pixels painted */
				&countOfPixelsPainted,             /* output:  count of pixels painted */
				10,					/* step size (mm) at which the center of plate is moved through iterations */
				CUP_SEGMENT_LABEL
			)
	)
	{
		postProcessRegionGrowing(imageOfWeightsInGrams, SenselIndices, &countOfPixelsPainted, CUP_SEGMENT_LABEL);
	}

	/********************* find the region under the bowl *********************/
	if(
			getRegionUnderObject(
				imageOfWeightsInGrams,			/* image data */
				Regions,  /* segmentation labels */
				0,      /* cup is on the right. Bowl is on the left */
				5,       /* cup is on the right. Bowl is on the left */
				BOWL_DIAMETER,     /* object diameter in mm */
				SenselIndices,           /* output:  indices of pixels painted */
				&countOfPixelsPainted,             /* output:  count of pixels painted */
				10,					/* step size (mm) at which the center of plate is moved through iterations */
				BOWL_SEGMENT_LABEL
			)
	)
	{
		postProcessRegionGrowing(imageOfWeightsInGrams, SenselIndices, &countOfPixelsPainted, BOWL_SEGMENT_LABEL);
	}


} // SegmentRegions







// sensel origion is the upper left point
inline Point getOrigionOfSensel(int sensRow, int sensCol) {
	Point origion;
	origion.x = senselOrigion[sensRow][sensCol][0]; // x
	origion.y = senselOrigion[sensRow][sensCol][1]; // y
	return origion;
}
inline float distance(Point a, Point b )
{
	// Calculating distance
    float retVal = sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2) * 1.0);
	return retVal;
}
//************************
inline bool isTouchingSide(
	const SenselSides side,
	const Point plateCenter,
	const Point so,
	const int gSS,
	const int dishRadius)
	{
		int startX, endX, startY, endY;
		startX= endX= startY= endY=-1;
		switch (side) {
			case U_S:
			startX = so.x;
			endX = so.x+gSS;
			startY = so.y;
			endY = so.y;
			break;
			case R_S:
			startX = so.x+gSS;
			endX = so.x+gSS;
			startY = so.y;
			endY = so.y+gSS;
			break;
			case B_S:
			startX = so.x;
			endX = so.x+gSS;
			startY = so.y+gSS;
			endY = so.y+gSS;
			break;
			case L_S:
			startX = so.x;
			endX = so.x;
			startY = so.y;
			endY = so.y+gSS;
			break;
		}
		// loop on the side
		for (int r = startY; r <= endY; r++) {
			for (int c = startX; c <= endX; c++) {
				Point point={c, r};
				if (distance(point, plateCenter) <= dishRadius) {
					return true;
				}
			}
		}

		return false;
	}

inline bool getRegionUnderObject(
	double *image,			/* image data */
  unsigned char *labels,  /* segmentation labels */
	int startAtColumn,      /* cup is on the right. Bowl is on the left */
	int stopAtColumn,       /* cup is on the right. Bowl is on the left */
  int objectDiameter,     /* object diameter in mm */
  int *indices,           /* output:  indices of pixels painted */
  int *count,             /* output:  count of pixels painted */
	int stepSize,					/* step size (mm) at which the center of plate is moved through iterations */
	unsigned char objectLabel
	)
	{
		*count = 0;
		bool retVal = false;
		/*make binary image*/
		bool binaryImage[NUMBER_OF_GRID_SENSELS];
		for (int ind = 0; ind < NUMBER_OF_GRID_SENSELS; ind++) {
			binaryImage[ind] = image[ind] > MIN_WEIGHT_TO_JOIN_REGION ? 1 : 0;
		}
		/**
		 * loop for all sensels. Use only 8 neighbors connected for the plate.
		 * step size to move the plate on this sensel.
		 * Assuming objects are within the sensing area
		 */

		/*for computation efficiency we skip tiles at the edge depending on the object size*/
		const int skipTiles = objectDiameter/2/(SENSEL_SIDE_LENGTH_MM+SPACING_BETWEEN_ADJACENT_TILES_MM);
		for (int row = skipTiles; row < NUMBER_OF_GRID_ROWS-skipTiles; row++) {
			for (int col = startAtColumn+skipTiles; col < stopAtColumn-skipTiles; col++) {
				/*skip this sensel if it is already labled*/
				if(labels[row*NUMBER_OF_GRID_COLUMNS+col]>0 ) {continue;}
				/*skip this sensel if it does not have weight on it*/
				if (!binaryImage[row*NUMBER_OF_GRID_COLUMNS+col]) {continue;}

				/*check limits*/
				if (row<1 || row>(NUMBER_OF_GRID_ROWS-2) || col<1 || col>(NUMBER_OF_GRID_COLUMNS-2)) continue;

				int neighbors = binaryImage[row*NUMBER_OF_GRID_COLUMNS+col]						/*the sensel itself*/
											+ binaryImage[row*NUMBER_OF_GRID_COLUMNS+col-1]				/*left*/
											+ binaryImage[(row-1)*NUMBER_OF_GRID_COLUMNS+col-1]		/*upper left*/
											+ binaryImage[(row-1)*NUMBER_OF_GRID_COLUMNS+col]			/*upper*/
											+ binaryImage[(row-1)*NUMBER_OF_GRID_COLUMNS+col+1]		/*upper right*/
											+ binaryImage[row*NUMBER_OF_GRID_COLUMNS+col+1]				/*right*/
											+ binaryImage[(row+1)*NUMBER_OF_GRID_COLUMNS+col+1]		/*bottom right*/
											+ binaryImage[(row+1)*NUMBER_OF_GRID_COLUMNS+col]			/*bottom*/
											+ binaryImage[(row+1)*NUMBER_OF_GRID_COLUMNS+col-1]		/*bottom left*/
											;
				/*use 8 neighbors for big objects that must cover at least 9 tiles such as plate. Using 51X51 mm tiles, the minimum number of tiles touched by 120 mm diameter object is 9 */
				if (objectDiameter > 120 && neighbors==9) /*8 neighbors + the sensel itself*/
				{
					moveObjectOverSensel(
						binaryImage,				/* binary image */
					  labels,  						/* segmentation labels */
						startAtColumn,      /* cup is on the right. Bowl is on the left */
						stopAtColumn,       /* cup is on the right. Bowl is on the left */
					  objectDiameter,     /* object diameter in mm */
					  indices,           	/* output:  indices of pixels painted */
					  count,             	/* output:  count of pixels painted */
						stepSize,						/* step size (mm) at which the center of plate is moved through iterations */
						row,								/*coordinates of the sensel over which the object is moving */
						col									/*coordinates of the sensel over which the object is moving */
					);
					if ((*count)>=16) { /*16 tiles are the min number of tiles that plate can rest on*/
						retVal=true;
					}
					/*exit whenever we can get the maximum number of possible tiles that can be touched by the plate*/
					if ((*count)>=22) {
						retVal=true;
						break;
					}
				}
				/*use at least two neighbors for bowl and cup*/
				else if (objectDiameter < 120 && neighbors>2) /*2 neighbors + the sensel itself*/
				{
					moveObjectOverSensel(
						binaryImage,				/* binary image */
					  labels,  						/* segmentation labels */
						startAtColumn,      /* cup is on the right. Bowl is on the left */
						stopAtColumn,       /* cup is on the right. Bowl is on the left */
					  objectDiameter,     /* object diameter in mm */
					  indices,           	/* output:  indices of pixels painted */
					  count,             	/* output:  count of pixels painted */
						stepSize,						/* step size (mm) at which the center of plate is moved through iterations */
						row,								/*coordinates of the sensel over which the object is moving */
						col									/*coordinates of the sensel over which the object is moving */
					);
					if (objectLabel==BOWL_SEGMENT_LABEL && (*count)>=4) {  /*4 tiles are the min number of tiles that the bowl can rest on*/
						retVal=true;
					} else if (objectLabel==CUP_SEGMENT_LABEL && (*count)>=3) {  /*3 tiles are the min number of tiles that the cup can rest on*/
						retVal=true;
					}
				}

				/*check this sensel*/
			}
		}

		return retVal;
	}




inline bool moveObjectOverSensel(
	bool *binaryImage,			/* binary image */
  unsigned char *labels,  /* segmentation labels */
	int startAtColumn,      /* cup is on the right. Bowl is on the left */
	int stopAtColumn,       /* cup is on the right. Bowl is on the left */
  int objectDiameter,     /* object diameter in mm */
  int *indices,           /* output:  indices of pixels painted */
  int *count,             /* output:  count of pixels painted */
	int stepSize,						/* step size (mm) at which the center of plate is moved through iterations */
	int row,								/*coordinates of the sensel over which the object is moving */
	int col									/*coordinates of the sensel over which the object is moving */
	)
{


	const float objectRadius = (objectDiameter/2);
	int numOfSenselsUnderDishFootprint  = 0;
	int localIndices[NUMBER_OF_GRID_SENSELS]; /*index of first tile found is saved at localIndices[0]*/
	const float SC2SC = sqrt(pow(SENSEL_SIDE_LENGTH_MM+SPACING_BETWEEN_ADJACENT_TILES_MM,2)+pow(SENSEL_SIDE_LENGTH_MM+SPACING_BETWEEN_ADJACENT_TILES_MM,2))/2; // sensel corner to sensel center
	Point sOrigion = getOrigionOfSensel(row,col);// sensel origion (upper left)
	Dishware object = {objectRadius, sOrigion}; //
	/*move this object accross this sensel and search for the tiles on which the object rests*/
	for (int obRow = sOrigion.y; obRow <= sOrigion.y+SENSEL_SIDE_LENGTH_MM+SPACING_BETWEEN_ADJACENT_TILES_MM; obRow+=stepSize) {
		for (int obCol = sOrigion.x; obCol <= sOrigion.x+SENSEL_SIDE_LENGTH_MM+SPACING_BETWEEN_ADJACENT_TILES_MM; obCol+=stepSize) {
			object.center.x = obCol;
			object.center.y = obRow;
			numOfSenselsUnderDishFootprint  = 0;
			memset(localIndices, 0, sizeof(int)*NUMBER_OF_GRID_SENSELS);
			/*Now loop for all tiles to find which one is under this object*/
			for (int gRow = 0; gRow < NUMBER_OF_GRID_ROWS; gRow++) {
				for (int gCol = startAtColumn; gCol < stopAtColumn; gCol++) {
					/*skip this sensel if it is already labled*/
					if(labels[gRow*NUMBER_OF_GRID_COLUMNS+gCol]>0 ) {continue;}
					/*skip this sensel if it does not have weight on it*/
					if (!binaryImage[gRow*NUMBER_OF_GRID_COLUMNS+gCol]) {continue;}
					/*get origion of the current grid sensel using its coordinates gRow and gCol*/
					Point gsOrigion = getOrigionOfSensel(gRow,gCol);
					Point
					gsCenter = {gsOrigion.x + SENSEL_SIDE_LENGTH_MM/2, gsOrigion.y + SENSEL_SIDE_LENGTH_MM/2}, /* center of the current sensel */
					gsUpperRight = {gsOrigion.x + SENSEL_SIDE_LENGTH_MM, gsOrigion.y},
					gsBottomRight = {gsOrigion.x + SENSEL_SIDE_LENGTH_MM, gsOrigion.y + SENSEL_SIDE_LENGTH_MM},
					gsBottomLeft = {gsOrigion.x, gsOrigion.y + SENSEL_SIDE_LENGTH_MM};
					/**
					 * initially skip if the distance between the sensel center and the
					 * object center is larger than {(the distance between sensel center and
					 * sensel corner) + dishware radius}
					 */
					if (distance(object.center, gsCenter) > (objectRadius+SC2SC)) {	continue;}
					/**
					 * check if the center of the current sensel exists under the plate
					 * circumference
					 */
					else if (
						/*check if the center of the current sensel exists under the object circumference*/
						distance(object.center, gsCenter) <= objectRadius
						/*check if upper left corner exists under the object*/
					|| distance(object.center, gsOrigion) <= objectRadius
					/*check if upper right corner exists under the object*/
					|| distance(object.center, gsUpperRight) <= objectRadius
					/*check if bottom right corner exists under the object*/
					|| distance(object.center, gsBottomRight) <= objectRadius
					/*check if bottom left corner exists under the object*/
					|| distance(object.center, gsBottomLeft) <= objectRadius
					/**/
					)
					{
						localIndices[numOfSenselsUnderDishFootprint]=gRow*NUMBER_OF_GRID_COLUMNS+gCol;
						numOfSenselsUnderDishFootprint++;
						// continue;
					}
					/*check angles*/
					else
					{
						/**
						 * This is the angle from the perspective of the sensel looking at
						 * the center of the object (the food container). Note that I use
						 * the upper left of the whole grid as the origion point 0,0 of
						 * the image as the convention in image processing in linux.
						 * Therefore the function atan2(-Y,X) gives this angle.
						 */
						float angle = atan2(-1*(object.center.y-gsCenter.y),(object.center.x-gsCenter.x+DBL_EPSILON)) * 180 / M_PI;
						if (angle <= 135 && angle > 45) {
							// upper side
							if(isTouchingSide(U_S, object.center, gsOrigion, SENSEL_SIDE_LENGTH_MM, objectRadius))
							{
								localIndices[numOfSenselsUnderDishFootprint]=gRow*NUMBER_OF_GRID_COLUMNS+gCol;
								numOfSenselsUnderDishFootprint++;
								// continue;
							}
						} else if (angle <= 45 && angle > -45) {
							// right side
							if(isTouchingSide(R_S, object.center, gsOrigion, SENSEL_SIDE_LENGTH_MM, objectRadius))
							{
								localIndices[numOfSenselsUnderDishFootprint]=gRow*NUMBER_OF_GRID_COLUMNS+gCol;
								numOfSenselsUnderDishFootprint++;
								// continue;
							}
						} else if (angle <= -45 && angle > -135) {
							// bottom side
							if(isTouchingSide(B_S, object.center, gsOrigion, SENSEL_SIDE_LENGTH_MM, objectRadius))
							{
								localIndices[numOfSenselsUnderDishFootprint]=gRow*NUMBER_OF_GRID_COLUMNS+gCol;
								numOfSenselsUnderDishFootprint++;
								// continue;
							}
						} else if (angle <= -135 || angle > 135) {
							// left side
							if(isTouchingSide(L_S, object.center, gsOrigion, SENSEL_SIDE_LENGTH_MM, objectRadius))
							{
								localIndices[numOfSenselsUnderDishFootprint]=gRow*NUMBER_OF_GRID_COLUMNS+gCol;
								numOfSenselsUnderDishFootprint++;
								// continue;
							}
						} else
						{
							// dishware not touching the sensel
							continue;
						}
					}// end of else
				}//for gCol
			}// for gRow
			/**/
			if (numOfSenselsUnderDishFootprint>=(*count)) /*adding the equal sign allow the segmentor to move to right if it finds the same number on the right*/
			{
				(*count) = numOfSenselsUnderDishFootprint;
				memcpy(indices, localIndices, NUMBER_OF_GRID_SENSELS*sizeof(int));
			}
		}// for obCol
	}// for obRow

	return true;
}

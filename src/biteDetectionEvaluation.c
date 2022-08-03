
/**
By Mohammad Mayyan, August 1st, 2022
*/

#include "smartDiningTable.h"

/*
** This program compares an automatically detected set of bites
** against a ground truth set and determines the number of
** true detections, false positives, and missed bites.
*/
void computeF1Score_Hoover(
double **computerDetectedWeights,
bool useConsumedBitesForGroundTruth,	// consumed = 1, picked up = 0
bool extendedOutput,										// yes = 1, no = 0
bool importantExtendedOutput										// yes = 1, no = 0
)
{

int	icntr,jcntr;
int	total_gt_bites[NUMBER_OF_OBJECTS_TO_TRACK]={0,0,0};
int	gt_bite_indices[NUMBER_OF_OBJECTS_TO_TRACK][MAX_BITES]; /*consumed*/

int	total_comp_bites[NUMBER_OF_OBJECTS_TO_TRACK]={0,0,0};
int	comp_bite_indices[NUMBER_OF_OBJECTS_TO_TRACK][MAX_BITES];
double	totalWeight_comp_bites[NUMBER_OF_OBJECTS_TO_TRACK]={0.0f,0.0f,0.0f};

int	gt_matched[NUMBER_OF_OBJECTS_TO_TRACK][MAX_BITES], comp_matched[NUMBER_OF_OBJECTS_TO_TRACK][MAX_BITES];
int	TP[NUMBER_OF_OBJECTS_TO_TRACK],FP[NUMBER_OF_OBJECTS_TO_TRACK],FN[NUMBER_OF_OBJECTS_TO_TRACK];	/*U is unmatched*/
int	comp_sec[NUMBER_OF_OBJECTS_TO_TRACK],gt_sec[NUMBER_OF_OBJECTS_TO_TRACK],early_sec[NUMBER_OF_OBJECTS_TO_TRACK];
int	mean_time_diff[NUMBER_OF_OBJECTS_TO_TRACK],stddev_time_diff[NUMBER_OF_OBJECTS_TO_TRACK];


//
long long int momentOfFirstGroundTruth=LLONG_MAX,
momentOfLastGroundTruth=LLONG_MIN;

/*****************************************************************/
/******************* ground truth ***********************************/
/*****************************************************************/
for (int64_t frm = 0; frm < numberOfDataFrames; frm++)
{
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    int GTV = retrievedGroundTruth[obj][frm];
    if (useConsumedBitesForGroundTruth)
    {
      if(	GTV == GROUND_TRUTH_LABELS_BITE_CONSUMED
        || GTV == GROUND_TRUTH_LABELS_MASSBITE_CONSUMED
        || GTV == GROUND_TRUTH_LABELS_DRINK_CONSUMED
      ) /*consumed*/
      {
        gt_bite_indices[obj][total_gt_bites[obj]]=frm;
        total_gt_bites[obj]++;

        if (frm < momentOfFirstGroundTruth) momentOfFirstGroundTruth = frm;
        if (frm > momentOfLastGroundTruth) momentOfLastGroundTruth = frm;
      }
    }
    else
    {
      if(	GTV == GROUND_TRUTH_LABELS_BITE_PICKED_UP
        || GTV == GROUND_TRUTH_LABELS_MASSBITE_PICKED_UP
        || GTV == GROUND_TRUTH_LABELS_DRINK_PICKED_UP
      ) /*picked up*/
      {
        gt_bite_indices[obj][total_gt_bites[obj]]=frm;
        total_gt_bites[obj]++;

        if (frm < momentOfFirstGroundTruth) momentOfFirstGroundTruth = frm;
        if (frm > momentOfLastGroundTruth) momentOfLastGroundTruth = frm;
      }
    }
  }
}
/****************************************************************/
printf("\n\tMeal duration based on GT for all objects =  %.1lf\n\n", (double)(momentOfLastGroundTruth-momentOfFirstGroundTruth)/(double)SAMPLE_RATE_OF_THE_SYSTEM);
/*****************************************************************/
/*****************************************************************/
/*****************************************************************/
if (importantExtendedOutput)
{
  for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
    printf("obj=%d, %d bites in GT\n", obj, total_gt_bites[obj]);
}
/*****************************************************************/
/******************* computer detections ***********************************/
/*****************************************************************/
for (int64_t frm = 0; frm < numberOfDataFrames; frm++)
{
  /* scan/skip forward if it is earlier than 10sec prior to first GT bite for all objects */
  int tempCounter = 0;
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    if (frm < gt_bite_indices[obj][0]-WINDOW_SIZE_FOR_MATCHING) tempCounter++;
  }
  if (tempCounter==NUMBER_OF_OBJECTS_TO_TRACK) {
    continue;	/* don't evaluate this md */
  }
  /* is it later than 10sec after last GT bite for all objects? scan/skip forward */
  tempCounter = 0;
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    if (frm > gt_bite_indices[obj][total_gt_bites[obj]-1]+WINDOW_SIZE_FOR_MATCHING) tempCounter++;
  }
  if (tempCounter==3) {
    continue;	/* don't evaluate this md */
  }
  /*************************************************************************/
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    if(computerDetectedWeights[obj][frm] != 0)
    {
      comp_bite_indices[obj][total_comp_bites[obj]]=frm;
      total_comp_bites[obj]++;
      totalWeight_comp_bites[obj] += computerDetectedWeights[obj][frm];
    }
  }
}
/*****************************************************************/
for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  totalWeightOfConsumedFood[obj] = (float)totalWeight_comp_bites[obj];
  /**********************/
  comp_sec[obj]=(comp_bite_indices[obj][total_comp_bites[obj]-1]-comp_bite_indices[obj][0]+2*WINDOW_SIZE_FOR_MATCHING)/SAMPLE_RATE_OF_THE_SYSTEM;
  gt_sec[obj]=(gt_bite_indices[obj][total_gt_bites[obj]-1]-gt_bite_indices[obj][0]+2*WINDOW_SIZE_FOR_MATCHING)/SAMPLE_RATE_OF_THE_SYSTEM;
  if(total_comp_bites[obj]>3)
  {
    early_sec[obj]=(comp_bite_indices[obj][3]-comp_bite_indices[obj][0])/SAMPLE_RATE_OF_THE_SYSTEM;
  }
  biteRate_spb[obj] = (double)comp_sec[obj]/(double)total_comp_bites[obj];
}
/*****************************************************************/
if (importantExtendedOutput)
{
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
    printf("obj=%d, %d bites in computer file, %0.1lf total weight, %0.1lf average bite size \n", obj, total_comp_bites[obj], totalWeight_comp_bites[obj], totalWeight_comp_bites[obj]/(double)total_comp_bites[obj]);

}
/*****************************************************************/
if (importantExtendedOutput)
{
  for (int obj = 0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    printf("[obj]=%d, \t computer meal duration %d sec (%0.1lf min), %.1lf spb, GT meal duration %d sec (%0.1lf min), %.1lf spb   early %.1lf spb\n",
      obj, comp_sec[obj], (double)comp_sec[obj]/60.0f, (double)comp_sec[obj]/(double)total_comp_bites[obj],
      gt_sec[obj], (double)gt_sec[obj]/60.0f, (double)gt_sec[obj]/(double)total_gt_bites[obj],
      (double)early_sec[obj]/(double)4.0);
  }
}
/*****************************************************************/

/* initialize all comp<->gt bite matches */
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  for (icntr=0; icntr<total_gt_bites[obj]; icntr++)
  {
    gt_matched[obj][icntr]=NOT_MATCHED;
  }
}

for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  for (icntr=0; icntr<total_comp_bites[obj]; icntr++)
  {
    comp_matched[obj][icntr]=NOT_MATCHED;
  }
}
/*****************************************************************/
  /* find matches from comp<->gt */
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  for (icntr=0; icntr<total_comp_bites[obj]; icntr++)
  {
    /* find first GT bite that is not yet matched but occurs
    ** after the previous computer detected bite */
    for (jcntr=0; jcntr<total_gt_bites[obj]; jcntr++)
    {
      if (gt_matched[obj][jcntr] == NOT_MATCHED  &&	(icntr == 0  ||  gt_bite_indices[obj][jcntr] > comp_bite_indices[obj][icntr-1]) )
      {
        break;
      }
    }

    if (jcntr == total_gt_bites[obj])
    {
      continue;	/* no GT bite can possibly match */
    }

    /* check if this GT bite occurs before next comp detection */
    if (icntr == total_comp_bites[obj]-1  ||  gt_bite_indices[obj][jcntr] < comp_bite_indices[obj][icntr+1])
    {		/* ok, match them */
      comp_matched[obj][icntr]=gt_bite_indices[obj][jcntr];
      gt_matched[obj][jcntr]=comp_bite_indices[obj][icntr];
    }
  }
}

/*****************************************************************/
if (extendedOutput)
{
  for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
  {
    for (icntr=0; icntr<total_comp_bites[obj]; icntr++)
    {
      printf("obj=%d,\t MD(%d) => GT(%d)\n", obj, comp_bite_indices[obj][icntr], comp_matched[obj][icntr]);
    }
  }
}

/*****************************************************************/
  /* count up TP, FP and FN */
  /* also calculate mean and stddev (absolute value)
  ** time diff of TP detections */
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  TP[obj]=FP[obj]=FN[obj]=0;
  mean_time_diff[obj]=0;
  for (icntr=0; icntr<total_gt_bites[obj]; icntr++)
  {
    if (gt_matched[obj][icntr] != NOT_MATCHED  &&  gt_matched[obj][icntr] != NOT_COUNTED)
    {
      TP[obj]++;
      if (extendedOutput)
      {
        printf("obj=%d,\t %d %d  %d\n", obj, gt_bite_indices[obj][icntr], gt_matched[obj][icntr], gt_matched[obj][icntr]-gt_bite_indices[obj][icntr]);
        mean_time_diff[obj]+=abs(gt_matched[obj][icntr]-gt_bite_indices[obj][icntr]);
      }
    }
    else if (gt_matched[obj][icntr] != NOT_COUNTED)
    {
      FN[obj]++;
    }
  }
}
/*****************************************************************/
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  if (TP[obj] > 0)
  {
    mean_time_diff[obj]/=TP[obj];
    for (icntr=0; icntr<total_gt_bites[obj]; icntr++)
    {
      if (gt_matched[obj][icntr] != NOT_MATCHED  &&  gt_matched[obj][icntr] != NOT_COUNTED)
      {
        stddev_time_diff[obj]+=SQR(abs(gt_matched[obj][icntr]-gt_bite_indices[obj][icntr])-mean_time_diff[obj]);
      }
    }
    stddev_time_diff[obj]=(int)(sqrt((double)stddev_time_diff[obj])/(double)(TP[obj]-1));
  }
  else
  {
    stddev_time_diff[obj]=0.0;
  }
}
/*****************************************************************/
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  for (icntr=0; icntr<total_comp_bites[obj]; icntr++)
  {
    if (comp_matched[obj][icntr] == NOT_MATCHED)
    {
      FP[obj]++;
    }
  }
}
/*****************************************************************/
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  if (extendedOutput)
  {
    printf("\tobj=%d,\t TP=%d  FP=%d  FN=%d\n", obj, TP[obj] ,FP[obj] ,FN[obj]);
    printf("\tobj=%d,\t mean_TP_diff=%d  stddev=%d\n", obj, mean_time_diff[obj], stddev_time_diff[obj]);
  }
  else
  {
    printf("\tobj=%d,\t TP=%d  FP=%d  FN=%d\n", obj, TP[obj] ,FP[obj] ,FN[obj]);
  }
}
/*****************************************************************/
int total_TP = 0, total_FP = 0, total_FN = 0;
for(int obj=0; obj < NUMBER_OF_OBJECTS_TO_TRACK; obj++)
{
  total_TP += TP[obj];
  total_FP += FP[obj];
  total_FN += FN[obj];
}
double F1_Score = (double)2*total_TP/(double)(2*total_TP+total_FP+total_FN);
printf("\tF1-score \t%lf \ttotal_TP \t%d \ttotal_FP \t%d \ttotal_FN \t%d\n", F1_Score, total_TP , total_FP ,total_FN);


} // end of computeF1Score_Hoover()

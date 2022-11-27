//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Piyush";
const char *studentID = "A59003976";
const char *email = "p1yadav@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// GShare
uint8_t *gshare_bht;
uint64_t ghistory_reg;

// Tournament
uint8_t *local_pred_table;
uint8_t *local_hist_table;
uint8_t *global_pred_table;
uint8_t *choice_table;

//
// TODO: Add your own Branch Predictor data structures here
//

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

// Initialize the predictor
//
void init_gshare()
{

  // printf("Initialising Gshare branch predictor...\n");
  uint32_t bht_total_count = 1 << ghistoryBits;
  gshare_bht = (uint8_t *)malloc(bht_total_count * sizeof(uint8_t));

  for (int i = 0; i < bht_total_count; i++)
  {
    gshare_bht[i] = WN;
  }

  ghistory_reg = 0;
}

uint32_t gshare_get_idx(uint32_t pc)
{
  uint32_t bht_total_count = 1 << ghistoryBits;
  uint32_t mask = bht_total_count - 1;

  uint32_t pc_lower = pc & mask;
  uint32_t ghistory_lower = ghistory_reg & mask;
  uint32_t idx = pc_lower ^ ghistory_lower;

  return idx;
}

uint8_t gshare_predict(uint32_t pc)
{
  // printf("Predicting from Gshare branch predictor...\n");

  uint32_t idx = gshare_get_idx(pc);

  switch (gshare_bht[idx])
  {
  case WN:
  case SN:
    return NOTTAKEN;
  case WT:
  case ST:
    return TAKEN;
  default:
    return NOTTAKEN;
  }
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_gshare(uint32_t pc, uint8_t outcome)
{
  uint32_t idx = gshare_get_idx(pc);

  switch (gshare_bht[idx])
  {
  case WN:
    gshare_bht[idx] = outcome == TAKEN ? WT : SN;
    break;
  case SN:
    gshare_bht[idx] = outcome == TAKEN ? WN : SN;
    break;
  case WT:
    gshare_bht[idx] = outcome == TAKEN ? ST : WN;
    break;
  case ST:
    gshare_bht[idx] = outcome == TAKEN ? ST : WT;
    break;
  }

  ghistory_reg = (outcome | (ghistory_reg << 1));
}

void cleanup_gshare()
{
  free(gshare_bht);
}

// Tournament initilization function
void init_tournament()
{
  ghistory_reg = 0;

  // // make a mask for pc, local_pht, ghistory,
  // uint32_t pht_entries = 1 << pcIndexBits;
  // pcmask = pht_entries - 1;

  // // get lower ghistoryBits
  // uint32_t mask = 1 << lhistoryBits;
  // lmask = mask - 1;

  // uint32_t bht_entries = 1 << ghistoryBits;
  // gmask = bht_entries - 1;

  // Local Pred table
  int local_pred_table_size = 1 << pcIndexBits;
  local_pred_table = (uint8_t *)malloc(sizeof(uint8_t) * local_pred_table_size);
  for (int i = 0; i < local_pred_table_size; i++)
  {
    local_pred_table[i] = WN;
  }

  int local_hist_table_size = 1 << lhistoryBits;
  local_hist_table = (uint8_t *)malloc(sizeof(uint8_t) * local_hist_table_size);
  for (int i = 0; i < local_hist_table_size; i++)
  {
    local_hist_table[i] = SN;
  }

  // Global PHT
  int s = 1 << ghistoryBits;
  global_pred_table = (uint8_t *)malloc(sizeof(uint8_t) * s);
  choice_table = (uint8_t *)malloc(sizeof(uint8_t) * s);
  for (int i = 0; i < s; i++)
  {
    global_pred_table[i] = WN;
    choice_table[i] = WT;
  }

  // // Choice PHT
  // size = 1 << ghistoryBits;
  // choice_bht = (uint32_t *)malloc(sizeof(uint32_t) * size);
  // for (int i = 0; i < size; i++)
  // {

  // }
}

uint8_t tournament_predict(uint32_t pc)
{

  // tournament
  uint32_t choice;
  uint32_t lhist;
  uint32_t prediction;

  int global_entries = 1 << ghistoryBits;

  uint32_t pc_lower_bits = pc & ((1 << pcIndexBits) - 1);

  uint32_t ghistbits = ghistory_reg & (global_entries - 1);
  choice = choice_table[ghistbits];

  if (choice < 2)
  {
    lhist = ((1 << lhistoryBits) - 1) & local_pred_table[pc_lower_bits];
    prediction = local_hist_table[lhist];
  }
  else
  {
    prediction = global_pred_table[ghistbits];
  }

  if (prediction > 1)
  {
    return TAKEN;
  }
  else
  {
    return NOTTAKEN;
  }
}

void train_tournament(uint32_t pc, uint8_t outcome)
{

  // tournament
  uint32_t ghistbits;
  uint32_t choice;
  uint32_t lhist;
  uint32_t lpred;
  uint32_t gpred;

  // printf("Global Mask : %u\nLocal Mask : %u\nPC Mask : %u\n",gmask,lmask,pcmask);

  // get lower pht index from pc
  uint32_t pc_lower_bits = pc & ((1 << pcIndexBits) - 1);

  // get ghistoryBits
  ghistbits = ghistory_reg & ((1 << ghistoryBits) - 1);
  choice = choice_table[ghistbits];

  lhist = ((1 << lhistoryBits) - 1) & local_pred_table[pc_lower_bits];
  lpred = local_hist_table[lhist];

  if (lpred > 1)
    lpred = TAKEN;
  else
    lpred = NOTTAKEN;

  gpred = global_pred_table[ghistbits];

  if (gpred > 1)
    gpred = TAKEN;
  else
    gpred = NOTTAKEN;

  if (gpred == outcome && lpred != outcome && choice_table[ghistbits] != 3)
    choice_table[ghistbits]++;
  else if (gpred != outcome && lpred == outcome && choice_table[ghistbits] != 0)
    choice_table[ghistbits]--;

  if (outcome == TAKEN)
  {
    if (global_pred_table[ghistbits] != 3)
      global_pred_table[ghistbits]++;
    if (local_hist_table[lhist] != 3)
      local_hist_table[lhist]++;
  }
  else
  {
    if (global_pred_table[ghistbits] != 0)
      global_pred_table[ghistbits]--;
    if (local_hist_table[lhist] != 0)
      local_hist_table[lhist]--;
  }

  local_pred_table[pc_lower_bits] = ((local_pred_table[pc_lower_bits] << 1) | outcome) & ((1 << lhistoryBits) - 1);
  ghistory_reg = ((ghistory_reg << 1) | outcome) & ((1 << ghistoryBits) - 1);
}

uint8_t make_prediction(uint32_t pc)
{
  //
  // TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    // init_tournament_gshare();
    break;
  default:
    break;
  }
}

void train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType)
  {
  case STATIC:
  case GSHARE:
    return train_gshare(pc, outcome);
  case TOURNAMENT:
    return train_tournament(pc, outcome);
  case CUSTOM:
    // return train_tournament_gshare(pc, outcome);
  default:
    break;
  }
}

void cleanup()
{
  switch (bpType)
  {
  case GSHARE:
    cleanup_gshare();
    break;
  case TOURNAMENT:
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

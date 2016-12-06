#ifndef CALC_WEIGHTS_H
#define CALC_WEIGHTS_H

/* Elements to use for the rank weight calculation */
#define MESO_ALERT "Meso_alert"
#define MESO_MAX   "Meso_max"
#define BWER_HGT   "BWERhgt_km"
#define BWER_VOL   "BWERvol_km3"
#define HAIL       "Hail_mm"
#define WDRAFT     "WDraft_kmh"
#define VILDENS    "VILDens"
#define MAXZ       "MaxZ"
#define DBZ45      "ETop45_km"
#define POSH       "POSH_pct"
#define MESH       "MESH_mm"

extern LOGICAL calc_data_rankweights (xmlNodePtr);

#endif /*CALC_WEIGHTS_H*/

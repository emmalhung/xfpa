/*========================================================================*/
/*
*   File:    resourceDefines.h
*
*   Purpose: This file contains all of the resource strings used in
*            the calls to XuGet<xxx>Resource() calls in the source.
*            The strings are defined as macros so that if any of the
*            resource names are changed in the resource file they
*            only need to be done here and are then propogated.
*
*   Note:    Not all resource names can be allocated here as some are
*            created dynamically. These cases in the code are limited
*            and should cause no great problem.
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*/
/*========================================================================*/

#ifndef RESOURCE_DEFINES_H
#define RESOURCE_DEFINES_H

#define RNallSetColorBg						".allSetColor.bg"
#define RNallSetColorFg						".allSetColor.fg"
#define RNalmostColorBg						".almostColor.bg"
#define RNalmostColorFg						".almostColor.fg"
#define RNanimationLoopDelay				".animationLoopDelay"
#define RNanimationMax						".animationMaxDelay"
#define RNarmColorBg						".armColorBg"
#define RNarmColorFg						".armColorFg"
#define RNaskBeforeImportingAllied			".askBeforeImportingAllied"
#define RNbackground						"*background"
#define RNborderColor						"*borderColor"
#define RNcreateBtnLabel					".createBtn.labelString"
#define RNcursorColors						".cursorColors"
#define RNdailyMatchFg                      ".dailyFieldTimeMatch.fg"
#define RNdailyMatchBg                      ".dailyFieldTimeMatch.bg"
#define RNdailySelectFg                     ".dailyFieldTimeSelect.fg"
#define RNdailySelectBg                     ".dailyFieldTimeSelect.bg"
#define RNdeleteFieldsDelete				".deleteFields.delete"
#define RNdeleteFieldsKeep					".deleteFields.keep"
#define RNdefaultFont                       ".defaultFont"
#define RNdefaultMapFonts                   ".defaultMapFonts"
#define RNeditContextMenu					"*.editContextMenu*.%s.labelString"
#define RNfcstTextGenerateColor				".fcstText.generateColor"
#define RNfieldHeader						"*.fieldHeader.labelString"
#define RNfieldInterpColorBg                ".fieldInterpColor.bg"
#define RNfieldInterpColorFg                ".fieldInterpColor.fg"
#define RNfieldUpdateDoneChar				".fieldUpdate.doneChar"
#define RNfieldUpdateExists					".fieldUpdate.exists"
#define RNfieldUpdateNoField				".fieldUpdate.noField"
#define RNfieldUpdateUpdate					".fieldUpdate.update"
#define RNfontsAvailable					".fontsAvailable"
#define RNfontsDescription					".fontsDescription"
#define RNfontSizeLabels					".fontSizeLabels"
#define RNfontSizesAvailable				".fontSizesAvailable"
#define RNforeground						"*foreground"
#define RNgenerateBtnLabel					".generateBtn.labelString"
#define RNgraphicProductsGenerateColor		".graphicProducts.generateColor"
#define RNguidanceAnimationSteps            ".guidanceAnimationSteps"
#define RNguidanceCheckInterval				".guidanceCheckInterval"
#define RNguidanceColours					".guidanceColours"
#define RNguidanceFieldMissing				".guidanceFieldMissing"
#define RNguidanceFieldRemoveDialogTitle	".guidanceFieldRemove.dialogTitle"
#define RNguidanceArrivedBg					".guidanceJustArrived.background"
#define RNguidanceArrivedFg					".guidanceJustArrived.foreground"
#define RNguidanceRunCaution				".guidanceRunCaution"
#define RNguidanceRunNA						".guidanceRunNA"
#define RNguidanceRunOk						".guidanceRunOk"
#define RNimagerySelectionOrder				".imagerySelectionOrder"
#define RNimagerySettingsOrder				".imagerySettingsOrder"
#define RNinterpPanelMaxGroupBtns			".interpPanel.maxGroupBtns"
#define RNinterpPanelMinFieldBtns			".interpPanel.minFieldBtns"
#define RNissueHeader						"*.issueHeader"
#define RNlevelHeader						"*.levelHeader.labelString"
#define RNloadBtnLabel						".loadBtn.labelString"
#define RNmailAddress						".mailAddress"
#define RNmaxContextMenuColumnLength		".maxContextMenuColumnLength"
#define RNmaxSampleItemListLen				".maxSampleItemListLength"
#define RNnoLinkColorBg						".noLinkColor.bg"
#define RNnoLinkColorFg						".noLinkColor.fg"
#define RNnoneditableTextBkgnd              ".noneditableTextField.background"
#define RNnotLinkableColorFg				".notLinkableColor.fg"
#define RNnotLinkableColorBg				".notLinkableColor.bg"
#define RNonlineHelp                        ".onlineHelp"
#define RNonlineHelpNewTab                  ".onlineHelp.newTab"
#define RNonlineHelpProgram					".onlineHelp.program"
#define RNonlineHelpLocationLabel           ".onlineLocLabel"
#define RNpartialLinkColorBg				".partialColor.bg"
#define RNpartialLinkColorFg				".partialColor.fg"
#define RNpointFcstGenerateColor			".pointFcst.generateColor"
#define RNproductRunningIndicatorBg			".productRunningIndicator.background"
#define RNproductRunningIndicatorFg			".productRunningIndicator.foreground"
#define RNproductRunningIndicatorSize		".productRunningIndicator.size"
#define RNpuckSizeDefault                   ".puckSizeDefault"
#define RNpuckSizeList                      ".puckSizeList"
#define RNradiusOIDefault			        ".radiusOfInfluenceDefault"
#define RNradiusOIList				        ".radiusOfInfluenceList"
#define RNradarRangeRingColor               ".radarRangeRingColor"
#define RNradarRangeLimitColor              ".radarRangeLimitColor"
#define RNsampleFontColors					".sampleFontColors"
#define RNsaveFieldVisibilityState			".saveFieldVisibilityState"
#define RNscratchpadLineColours				".scratchpadLineColours"
#define RNscratchpadTextColours				".scratchpadTextColours"
#define RNselectFgColor						"*selectFgColor"
#define RNselectBgColor						"*selectBgColor"
#define RNsendBtnLabel						".sendBtn.labelString"
#define RNsequenceIsFieldBg					".sequence.isField.bg"
#define RNsequenceIsFieldFg					".sequence.isField.fg"
#define RNsequenceMnFieldBg					".sequence.mnField.bg"
#define RNsequenceMnFieldFg					".sequence.mnField.fg"
#define RNsequenceNoFieldBg					".sequence.noField.bg"
#define RNsequenceNoFieldFg					".sequence.noField.fg"
#define RNnormalIncrementLabel				".normalIncrementLabel"
#define RNsequenceIncrements                ".sequenceIncrements"
#define RNsmoothingDefault                  ".lineSmoothingAmountDefault"
#define RNsmoothingList                     ".lineSmoothingAmountList"
#define RNsourceHeader						"*.sourceHeader"
#define RNsourceUpdatingFlashColour			".sourceUpdating.FlashColour"
#define RNsourceUpdatingFlashTime			".sourceUpdating.FlashTime"
#define RNsourceUpdatingIndicatorDelay		".sourceUpdating.IndicatorDelay"
#define RNstartBtn							".startBtn"
#define RNstopBtn							".stopBtn"
#define RNtimelinkPanelMaxGroupBtns			".timelinkPanel.maxGroupBtns"
#define RNtitle								".title"
#define RNT0roundOff						".T0.roundOff"
#define RNulToExpireLabel					".ul*.toExpire.labelString"
#define RNviewerTitle						".viewerTitle"
#define RNviewerUpdateInterval				".viewerUpdateInterval"
#define RNtextFieldBg						".XmTextField.background"
#define RNuserLicenseAboutToExpire			"*.ul*.toExpire.labelString"
#define RNuserLicenseAllInUse				"*.ul*.allInUse.labelString"
#define RNuserLicenseExpired				"*.ul*.expired.labelString"
#define RNuserLicenseLost					"*.ul*.lost.labelString"
#define RNuserLicenseNoLicense				"*.ul*.noLicense.labelString"
#define RNvalidHeader						"*.validHeader"

#endif

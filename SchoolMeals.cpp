/*================================================================
Copyright(C) 2012, The Urban Institute. All Rights Reserved.

Author: Michael Martinez-Schiferl
Date: August 27, 2012

Revision history:

<DESCRIPTION VERSION=1_0 MMS, 22-Sep_2013>
<li>Non-reporting high income households are now excluded from participation if they have annual
income above <em>HighIncomePctGuidelines</em>, are not eligible for free meals, and are only eligible
for reduced-price lunch in one month.  This version implements the third condition whereas version
0.8 excluded a high income child eligible for more than one month of reduced-price lunch too.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_8 MMS, 22-Sep_2013>
<li>Implemented RankLikelyRecipients() and SetParticipationDecision().</li>
<li>Added program variable list program rules <em>NumberHotMealsRecip</em> and <em>NumberFreeRedMealsRecip</em>.</li>
<li>Added program rule <em>HighIncomePctGuidelines</em>.</li>
<li>Added regular output variables <em>IsHighIncome</em>.</li>
<li>Added debug output variables <em>RcvdHotMeals</em>, <em>RcvdFreeReducedMeals</em>, and <em>SchoolMealsRank</em></li>
<li>Added summary tables B2, A1, S1, S2, and S3.</li>
<li>Cleaned up code.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_7 MMS, 28-Sep_2012>
<li>Simulates eligibility in school year 1 seperate from eligibility in school year 2.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_6 MMS, 28-Sep_2012>
<li>Removes an error present in the SAS School Meals simulation that assigned 1.85 times the poverty guideline increment for families with more than 8 persons.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_5 MMS, 27-Sep_2012>
<li>Adds summary table B1.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_4 MMS, 27-Sep_2012>
<li>This version duplicates the SAS simulated eligibility results.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_3 MMS, 21-Sep_2012>
<li>Implemented CSchoolMealsEligibility functions.  Problem with running to produce microdata results.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_2 MMS, 4-Sep_2012>
<li>Incremental implementation of SchoolMeals.  Includes declarations and empty methods for new SchoolMeals CEligibility, CBenefits, and CParticipation classes.</li>
</DESCRIPTION>

<DESCRIPTION VERSION=0_1 JWM, 27-Aug_2012>
<li>Initial SchoolMeals version setup.  Includes unit functions that are not called.
 May be implemented if needed.  If so, need to add ParentIDs program rule.</li>
</DESCRIPTION>
=================================================================*/

#include "..\..\TrimEXE\TrimDLL/stdafx.h"
#include "..\..\TrimEXE\TrimDLL/piter.h"
#include "..\..\TrimEXE\TrimDLL/hset.h"
#include <sqlext.h>
#include "..\..\TrimEXE\TrimDLL/genset.h"
#include "..\..\TrimEXE\TrimDLL/houseset.h"
#include "..\..\TrimEXE\TrimDLL/househol.h"
#include "..\..\TrimEXE\TrimDLL/packager.h"
#include "..\..\TrimEXE\TrimDLL/baseinst.h"
#include "..\..\TrimEXE\TrimDLL/inst.h"
#include "..\..\TrimEXE\TrimDLL/simulate.h"
#include "..\..\TrimEXE\TrimDLL/func.h"
#include "..\..\TrimEXE\TrimDLL/ssim.h"
#include "..\..\TrimEXE\TrimDLL/section.h"
#include "..\..\TrimEXE\TrimDLL/table.h"

#include "SchoolMeals.h"
#include <math.h>
#include "afxdllx.h"

using namespace std;

static AFX_EXTENSION_MODULE NEAR extensionDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		// Extension DLL one-time initialization - do not allocate memory here,
		//   use the TRACE or ASSERT macros or call MessageBox
		if (!AfxInitExtensionModule(extensionDLL, hInstance))
			return 0;

		// Other initialization could be done here, as long as
		// it doesn't result in direct or indirect calls to AfxGetApp.
		// This extension DLL doesn't need to access the app object
		// but to be consistent with testdll1.dll, this DLL requires
		// explicit initialization as well (see below).

		// This allows for greater flexibility later in development.
	}
	return 1;   // ok
}

// Exported DLL initialization is run in context of running application
extern "C" void WINAPI InitDllSim()
{
	// create a new CDynLinkLibrary for this app
	new CDynLinkLibrary(extensionDLL);
	// nothing more to do
}

//========================================================================
CSim *CSchoolMealsSim::CreateSim
  (CDatabase *pResultDatabase, const char *pSimulationName, const char *pSimulationID, CHousehold *pHouse) {
	  return new CSchoolMealsSim (pResultDatabase, pSimulationName, pSimulationID, pHouse);
}

/*========================================================================
CSchoolMealsResults
====*/
CSchoolMealsResults::CSchoolMealsResults
  (CDatabase *pResultDatabase, CSim *pParent, CHousehold *pHouse) : Result (((CSchoolMealsSim *)pParent)->Result),
  CResultSet (pResultDatabase, pParent, sizeof (CSchoolMealsMicroResults), pHouse) {
    pTempSchoolSimResult = new CSchoolMealsMicroResults;
    pTempResult = (CResultType *)pTempSchoolSimResult;
    pResultMonthlySet = new CSchoolMealsMonthlyResults (pResultDatabase, pParent, pHouse);
}
//========================================================================
CSchoolMealsResults::~CSchoolMealsResults() {
  delete pTempSchoolSimResult;
  delete pResultMonthlySet;
}
//========================================================================
void CSchoolMealsResults::Initialize() {
  Register();
  pResultMonthlySet->Initialize ();
}
//========================================================================
void CSchoolMealsResults::CopyResultRecord (char *&p) {
}
//========================================================================
void CSchoolMealsResults::Register() {
  pHousehold->RegisterVar_Int ("IsEverEligFreeMeals",	(int &)Result->IsEverEligFreeMeals, pHousehold->pExcecutingResultSet->BaseIdx);
  pHousehold->RegisterVar_Int ("IsEverEligRedPriceMeals",	(int &)Result->IsEverEligRedPriceMeals, pHousehold->pExcecutingResultSet->BaseIdx);
  pHousehold->RegisterVar_Int ("IsHighIncome",	(int &)Result->IsHighIncome, pHousehold->pExcecutingResultSet->BaseIdx);
  pHousehold->RegisterVar_Int ("RcvdFreeReducedMeals",	(int &)Result->RcvdFreeReducedMeals, pHousehold->pExcecutingResultSet->BaseIdx);
  pHousehold->RegisterVar_Int ("RcvdHotMeals",	(int &)Result->RcvdHotMeals, pHousehold->pExcecutingResultSet->BaseIdx);
  pHousehold->RegisterVar_Int ("SchoolMealsRank",	(int &)Result->SchoolMealsRank, pHousehold->pExcecutingResultSet->BaseIdx);
}
//========================================================================


/*========================================================================
CSchoolMealsMonthlyResults
====*/
CSchoolMealsMonthlyResults::CSchoolMealsMonthlyResults
  (CDatabase *pResultDatabase, CSim *pParent, CHousehold *pHouse) :
    Result (((CSchoolMealsSim *)pParent)->Result),
    CResultMonthlySet (pResultDatabase, pParent, pHouse) {
      pTempSchoolSimResult = (CSchoolMealsMicroResults *)pHousehold->pExcecutingResultSet->pTempResult;
}

//========================================================================
void CSchoolMealsMonthlyResults::Initialize () {
  Register();
}

//========================================================================
void CSchoolMealsMonthlyResults::CopyMonthRecord (int Month, char *&p) {
}

//========================================================================
void CSchoolMealsMonthlyResults::Register() {
  pHousehold->RegisterVar_Int    ("MonthlyEligibilityType", (int &)Result->MonthlyEligibilityType[11], pSim->pResultSet->BaseIdx, TRUE);
  pHousehold->RegisterVar_Single ("MonthlyEligSchoolMealSubsidies", Result->MonthlyEligSchoolMealSubsidies[11], pSim->pResultSet->BaseIdx, TRUE);
  pHousehold->RegisterVar_Int    ("MonthlyParticipationType", (int &)Result->MonthlyParticipationType[11], pSim->pResultSet->BaseIdx, TRUE);
  pHousehold->RegisterVar_Single ("MonthlyRcvdSchoolMealSubsidies", Result->MonthlyRcvdSchoolMealSubsidies[11], pSim->pResultSet->BaseIdx, TRUE);
}


/*========================================================================
CSchoolMealsSim
====*/
CSchoolMealsSim::CSchoolMealsSim 
  (CDatabase *pResultDatabase,
  const char *pSimulationName, 
  const char *pSimulationID, CHousehold *pHouse) : CSSim (pResultDatabase, pSimulationName, pSimulationID, pHouse) { 

		pUnit						= new CSchoolMealsUnit (this, pHouse);

		pEligible				= new CSchoolMealsEligible(this, pHouse);
		pBenefit				= new CSchoolMealsBenefit(this, pHouse);
		pParticipate		= new CSchoolMealsParticipate(this, pHouse);

		pResultSet			= new CSchoolMealsResults(pResultDatabase, this, pHouse);
		Result.Set(&pResultSet->Result);
		CreateTables(pResultDatabase);
		HouseholdProcessingHasBegun = false;
}

//========================================================================
CSchoolMealsSim::~CSchoolMealsSim() {
	if(pUnit != NULL) {
    delete pUnit;
    pUnit = NULL;
  }

  if(pEligible != NULL) {
    delete pEligible;
    pEligible = NULL;
  }
  
  if(pBenefit != NULL) {
    delete pBenefit;
    pBenefit = NULL;
  }

  if(pParticipate != NULL) {
    delete pParticipate;
    pParticipate = NULL;
  }

  if(pResultSet != NULL) {
    delete pResultSet;
    pResultSet = NULL;
  }
}

//========================================================================
void CSchoolMealsSim::Initialize() {
  pResultSet->Initialize();
  pUnit->Initialize();
  CFuncSim::Initialize();
	pEligible->Initialize();
  pBenefit->Initialize();
  pParticipate->Initialize();
}

//========================================================================
void CSchoolMealsSim::CheckRules () {
  //Check inputs and throw errors if not set
	pMonthlyEarnings->GetFirstVar();
  if (pMonthlyEarnings->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlyEarnings.";
		  AfxThrowUserException();
	}      

	pMonthlySsiBenefits->GetFirstVar();
  if (pMonthlySsiBenefits->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlySsiBenefits.";
		  AfxThrowUserException();
	}      

	pMonthlyTANFBenefits->GetFirstVar();
  if (pMonthlyTANFBenefits->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlyTANFBenefits.";
		  AfxThrowUserException();
	}   

	pMonthlyUnearnedIncome->GetFirstVar();
  if (pMonthlyUnearnedIncome->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlyUnearnedIncome.";
		  AfxThrowUserException();
	}

	pMonthlyUnitSNAPBenefits->GetFirstVar();
  if (pMonthlyUnitSNAPBenefits->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlyUnitSNAPBenefits.";
		  AfxThrowUserException();
	}

	pMonthlyUnitTANFBenefits->GetFirstVar();
  if (pMonthlyUnitTANFBenefits->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for MonthlyUnitTANFBenefits.";
		  AfxThrowUserException();
	}

  pNumberHotMealsRecip->GetFirstVar();
  if (pNumberHotMealsRecip->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for NumberHotMealsRecip.";
		  AfxThrowUserException();
	}   

	pNumberFreeRedMealsRecip->GetFirstVar();
  if (pNumberFreeRedMealsRecip->IsLastVar()) {
		  pHousehold->Error =
		  "Please specify a variable for NumberFreeRedMealsRecip.";
		  AfxThrowUserException();
	}   
}

//========================================================================
void CSchoolMealsSim::ListNationalInst () {
  RFX_Int ("SimulationMode", SimulationMode);
}

//========================================================================
void CSchoolMealsSim::ListStateInst(){
}

//========================================================================
void CSchoolMealsSim::MoveStateFields (int StateNumber){
}

void CSchoolMealsSim::ListVarInst() {
  RFX_Param ("MonthlyEarnings",					 pMonthlyEarnings);
  RFX_Param ("MonthlySsiBenefits",			 pMonthlySsiBenefits);
  RFX_Param ("MonthlyTANFBenefits",			 pMonthlyTANFBenefits);
  RFX_Param ("MonthlyUnearnedIncome",		 pMonthlyUnearnedIncome);
  RFX_Param ("MonthlyUnitSNAPBenefits",	 pMonthlyUnitSNAPBenefits);
  RFX_Param ("MonthlyUnitTANFBenefits",	 pMonthlyUnitTANFBenefits);
  RFX_Param ("NumberFreeRedMealsRecip",  pNumberFreeRedMealsRecip);
  RFX_Param ("NumberHotMealsRecip",	     pNumberHotMealsRecip);

	RFX_Func();
}

//========================================================================
void CSchoolMealsSim::Simulate() {
  int	 StateIdx = pHousehold->Record.FipsStateCode - 1;
  bool categoricalEligibility = false;
  bool monetaryEligibility = false;

  if (!HouseholdProcessingHasBegun) { 
  	/*Perform any final initialization before beginning household processing*/
		CheckRules();
		HouseholdProcessingHasBegun = true;
  }

  if( SimulationMode==BASELINE_SIM_MODE ) {
		//before performing person-level processing, we first call a few functions for some household-level processing
		if( pEligible->IsAnyoneCategoricallyEligible() ) {
			pEligible->SetMonetarilyEligible();

			//start person-level processing
			pUnit->GetFirstPerson();
			do {
				pEligible->SetCategoricallyEligible();
				pEligible->SetEligibility();
				pBenefit->SetPotentialBenefits();
			} while (pUnit->GetNextPerson());

			//assigning participation
			pParticipate->RankLikelyRecipients();
			pParticipate->SetParticipationDecision();
		}
  }
	else if( SimulationMode==ALTERNATE_SIM_MODE) {
		//Not yet implemented, throw error for now.
		pHousehold->Error ="The SchoolMeals module alternative mode has not yet been implemented.";
		AfxThrowUserException();
	}
}

//========================================================================
void CSchoolMealsSim::CreateTables (CDatabase *pDatabase) {
  int i = 0;
  if( BasicTables ) {
    pTable[i] = new CSchoolMealsTableB1 (pDatabase, this, pHousehold);
    i++;
    pTable[i] = new CSchoolMealsTableB2 (pDatabase, this, pHousehold);
    i++;
  }
  if( MonthlyTables ) {
  }
  if( DetailedTables ) {
  }
  if( StateTables ) {   
		pTable[i] = new CSchoolMealsTableS1 (pDatabase, this, pHousehold);
    i++;
		pTable[i] = new CSchoolMealsTableS2 (pDatabase, this, pHousehold);
    i++;
		pTable[i] = new CSchoolMealsTableS3 (pDatabase, this, pHousehold);
    i++;
  }
  if( AlignmentTables ) {  
    pTable[i] = new CSchoolMealsTableA1 (pDatabase, this, pHousehold);
    i++;
	}
}

//========================================================================
int CSchoolMealsSim::IncludeThisHousehold() {
  pIsHouseholdIncluded = pFuncFactory->pFuncMgr->GetFunc ("IsHouseholdIncluded");
  int IsIncluded = (int)pIsHouseholdIncluded->GetTotal (NULL);
  return IsIncluded;
}


///////////////////////////////////////////////////////////////////////////
// CSchoolMealsUnit
//==========================================================================
CSchoolMealsUnit::CSchoolMealsUnit (CSchoolMealsSim *pParent, CHousehold *pHouse) : 
    CUnit (pParent, pHouse),
		SMResult (pParent->Result),
		CInstSet (pHouse, pParent->SimulateName, pParent->SimulateID) {
    pSim = (CSchoolMealsSim *)pParent;
}

CSchoolMealsUnit::~CSchoolMealsUnit() {
}
//===========================================================
void CSchoolMealsUnit::Initialize() {
	FetchInst();
}
//===========================================================
void CSchoolMealsUnit::ListStateInst () {
}
//===========================================================
void CSchoolMealsUnit::MoveStateFields (int StateNumber) {
}
//===========================================================
void CSchoolMealsUnit::ListVarInst() {
}

//=====================================================================
BOOL CSchoolMealsUnit::GetFirstUnit() {
  UnitTypeTag UnitType;
  UnitType = CUnit::ONE_UNIT_PER_HOUSEHOLD;
  //UnitType = CUnit::ONE_UNIT_PER_FAMILY;
  SelectUnits (UnitType);
  //AdjustUnits();
  UnitNum = 0;
  GetFirstPerson();
  return TRUE;
};

//=====================================================================
void CSchoolMealsUnit::AdjustUnits () {
	pHousehold->GetFirstPerson();
	do {
	} while (pHousehold->GetNextPerson());
}
//=====================================================================
void CSchoolMealsUnit::MergeUnits(int UnitNumToDrop,int UnitNumToMergeInto) {
	for (int i = 0; i < pHousehold->HouseholdNumberofPersons; i++) {
		if (UnitArray[i] == UnitNumToDrop) {
			UnitArray[i] = UnitNumToMergeInto;
		}
	}
	for (int i = 0; i < pHousehold->HouseholdNumberofPersons; i++) {
		if (UnitArray[i] > UnitNumToDrop) {
			UnitArray[i] = UnitArray[i] - 1;
		}
	}
}

//=========================================================================


/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsEligible
CSchoolMealsEligible::CSchoolMealsEligible(CSchoolMealsSim *pParent, 
                            CHousehold *pHouse) : 
                            SMResult (pParent->Result),
                            CEligible (pParent, pHouse) {
  pSim = pParent;
};

//=========================================================================
CSchoolMealsEligible::~CSchoolMealsEligible() {
};

//=========================================================================
void CSchoolMealsEligible::ListNationalInst () {
  RFX_Single("FreeMealsPctGuidelines", FreeMealsPctGuidelines);
	RFX_Single("HighIncomePctGuidelines", HighIncomePctGuidelines);
  RFX_Single("ReducedPriceMealsPctGuidelines", ReducedPriceMealsPctGuidelines);
};

//=========================================================================
void CSchoolMealsEligible::ListStateInst() {
}

//=========================================================================
void CSchoolMealsEligible::MoveStateFields(int StateNumber) {
}

//=========================================================================
void CSchoolMealsEligible::ListVarInst(){
	//RFX_Func();
}

//=========================================================================
void CSchoolMealsEligible::ListStateBracketInst () {
}

//=========================================================================
void CSchoolMealsEligible::MoveStateBracketFields (int StateNumber, int BracketNumber) {
}

//=========================================================================
void CSchoolMealsEligible::ListBracketInst () {
  RFX_Single("SY1PovertyGuidelinesUS",SY1PovertyGuidelinesUS[MAX_ARRAY_SIZE-1]);
  RFX_Single("SY1PovertyGuidelinesAK",SY1PovertyGuidelinesAK[MAX_ARRAY_SIZE-1]);
  RFX_Single("SY1PovertyGuidelinesHI",SY1PovertyGuidelinesHI[MAX_ARRAY_SIZE-1]);
  RFX_Single("SY2PovertyGuidelinesUS",SY2PovertyGuidelinesUS[MAX_ARRAY_SIZE-1]);
  RFX_Single("SY2PovertyGuidelinesAK",SY2PovertyGuidelinesAK[MAX_ARRAY_SIZE-1]);
  RFX_Single("SY2PovertyGuidelinesHI",SY2PovertyGuidelinesHI[MAX_ARRAY_SIZE-1]);
}

//=========================================================================
void CSchoolMealsEligible::MoveBracketFields (int BracketNumber) {
  SY1PovertyGuidelinesUS[BracketNumber] = SY1PovertyGuidelinesUS[MAX_ARRAY_SIZE-1];
  SY1PovertyGuidelinesAK[BracketNumber] = SY1PovertyGuidelinesAK[MAX_ARRAY_SIZE-1];
  SY1PovertyGuidelinesHI[BracketNumber] = SY1PovertyGuidelinesHI[MAX_ARRAY_SIZE-1];
  SY2PovertyGuidelinesUS[BracketNumber] = SY2PovertyGuidelinesUS[MAX_ARRAY_SIZE-1];
  SY2PovertyGuidelinesAK[BracketNumber] = SY2PovertyGuidelinesAK[MAX_ARRAY_SIZE-1];
  SY2PovertyGuidelinesHI[BracketNumber] = SY2PovertyGuidelinesHI[MAX_ARRAY_SIZE-1];
}

//=========================================================================
void CSchoolMealsEligible::Initialize() {
  FetchInst();
}

//=========================================================================
void CSchoolMealsEligible::Calculate() {
}

//===================================================================
bool  CSchoolMealsEligible::IsAnyoneCategoricallyEligible() {
	//Checks to see if this household contains children who might receive school lunch
	bool isCategoricallyEligible = false;
  int currentPerson = pUnit->GetPersonNum();

  pUnit->GetFirstPerson();
  do {
		if ( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 ) {
			if ( pHousehold->Person->Age >= 16 && pHousehold->Person->HsCollegeAttendance!=1 ) {
				isCategoricallyEligible = true;
			}
			else
				isCategoricallyEligible = true;
    }
  } while( pUnit->GetNextPerson() );

  pUnit->SetPerson(currentPerson);

	return( isCategoricallyEligible );
}

//===================================================================
void  CSchoolMealsEligible::SetCategoricallyEligible() {
	//Checks the categorical eligibility of this person

	//initialize monthly household class variables to 0
	for( int iMonth=0; iMonth<12; iMonth++ ) {
		MonthlyCategoricalElig[iMonth]		= 0;
	}

	if( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 ) {
		if( pHousehold->Person->Age == 5) {
			MonthlyCategoricalElig[8]		=	1;
			MonthlyCategoricalElig[9]		=	1;
			MonthlyCategoricalElig[10]	=	1;
			MonthlyCategoricalElig[11]	=	1;
		}
		else if ( pHousehold->Person->Age >= 16 && pHousehold->Person->HsCollegeAttendance!=1 ) {
			MonthlyCategoricalElig[0]		=	1;
			MonthlyCategoricalElig[1]		=	1;
			MonthlyCategoricalElig[2]		=	1;
			MonthlyCategoricalElig[3]		=	1;
			MonthlyCategoricalElig[4]		=	1;		
		}
		else {
			MonthlyCategoricalElig[0]		=	1;
			MonthlyCategoricalElig[1]		=	1;
			MonthlyCategoricalElig[2]		=	1;
			MonthlyCategoricalElig[3]		=	1;
			MonthlyCategoricalElig[4]		=	1;		
			MonthlyCategoricalElig[8]		=	1;
			MonthlyCategoricalElig[9]		=	1;
			MonthlyCategoricalElig[10]	=	1;
			MonthlyCategoricalElig[11]	=	1;
		}
	}
}

//===================================================================
void  CSchoolMealsEligible::SetMonetarilyEligible() {  
	//Add up this households income and determine if they are monetarily eligible in any month
  int currentPerson = pUnit->GetPersonNum();
	float annualEarnings = 0;
	
	//initialize monthly household class variables to 0
	for( int iMonth=0; iMonth<12; iMonth++ ) {
		MonthlyHhMonetaryEligType[iMonth] = 0;
		MonthlyHhIncome[iMonth]						= 0;
	}

  pUnit->GetFirstPerson();
  do {
		//make sure variables are initialized
		pSim->pMonthlyEarnings->SumOfVar();
		pSim->pMonthlySsiBenefits->SumOfVar();
		pSim->pMonthlyTANFBenefits->SumOfVar();
		pSim->pMonthlyUnearnedIncome->SumOfVar();

		//first add up annual earnings for all persons under 18, in case we need it
		annualEarnings = 0;
		if( pHousehold->Person->Age <= 18 ) {
 		  for( int iMonth=0; iMonth<12; iMonth++ ) {
				annualEarnings += (*pSim->pMonthlyEarnings)[iMonth];
		  }
		}

		//add up household income
		for( int iMonth=0; iMonth<12; iMonth++ ) {
			if( pHousehold->Person->Age <= 18 && annualEarnings < 1250 ) {
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlySsiBenefits)[iMonth];
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlyTANFBenefits)[iMonth];
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlyUnearnedIncome)[iMonth];
			}
			else {
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlyEarnings)[iMonth];
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlySsiBenefits)[iMonth];
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlyTANFBenefits)[iMonth];
				MonthlyHhIncome[iMonth] += (*pSim->pMonthlyUnearnedIncome)[iMonth];
			}
		}
  } while( pUnit->GetNextPerson() );

  int numInFamily = GetNumPersonsInFamily();
	float annualHhIncome = 0;

  //determine household eligibility
	for( int iMonth=0; iMonth<12; iMonth++ ) {
		float povertyGuideline = GetPovertyGuideline(iMonth, numInFamily);

		if( MonthlyHhIncome[iMonth] < ((FreeMealsPctGuidelines/100)*povertyGuideline) ) {
			MonthlyHhMonetaryEligType[iMonth] = SM_FREE_MEALS;
		}
		else if ( MonthlyHhIncome[iMonth] < ((ReducedPriceMealsPctGuidelines/100)*povertyGuideline) ) {
			MonthlyHhMonetaryEligType[iMonth] = SM_REDUCED_PRICE_MEALS;
		}

		annualHhIncome += MonthlyHhIncome[iMonth];
	}

	//determine if household is considered 'high income'
	float highIncAnnualGuideline = GetPovertyGuideline(DECEMBER, numInFamily)*12;
	if( annualHhIncome < ((HighIncomePctGuidelines/100)*highIncAnnualGuideline) ) {
		SMResult->IsHighIncome=1;
	}

  pUnit->SetPerson(currentPerson);

}

//===================================================================
bool  CSchoolMealsEligible::IsSNAPReporterUnit(int month) {
	//returns true/false whether the unit received SNAP this month
	bool receivedSNAP = false;
	
	pSim->pMonthlyUnitSNAPBenefits->SumOfVar();
	if( (*pSim->pMonthlyUnitSNAPBenefits)[month] > 0 )
		receivedSNAP = true;

	return( receivedSNAP );
}

//===================================================================
bool CSchoolMealsEligible::IsTANFReporterUnit(int month) {
	//returns true/false whether the unit received TANF this month
	bool receivedTANF = false;
	
	pSim->pMonthlyUnitTANFBenefits->SumOfVar();
	if( (*pSim->pMonthlyUnitTANFBenefits)[month] > 0 )
		receivedTANF = true;

	return( receivedTANF );
}

//===================================================================
int CSchoolMealsEligible::GetNumPersonsInFamily() {
	//right now, this function is using the household as its definition for 'family'
	int numPersonsInFamily = 0;

  pUnit->GetFirstPerson();
  do {
		numPersonsInFamily++;
  } while( pUnit->GetNextPerson() );

	return( numPersonsInFamily );
}

//===================================================================
float CSchoolMealsEligible::GetPovertyGuideline(int month, int numberFamilyMembers) {
  float annualPovertyGuideline	= 0;
  float monthlyPovertyGuideline = 0;
	int state = pHousehold->Record.FipsStateCode;

	if( numberFamilyMembers < 1 || numberFamilyMembers > 20 ) {
    pHousehold->Error =	"CSchoolMealsEligible::GetPovertyGuideline: number of family members out of bounds.";
		AfxThrowUserException();
	}

	if( month >= 0 && month <= 6 ) { 
		//School Year 1
		if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
			annualPovertyGuideline = SY1PovertyGuidelinesUS[numberFamilyMembers-1];
		}
		else if ( state == STATE_FIPS_ALASKA ) {
			annualPovertyGuideline = SY1PovertyGuidelinesAK[numberFamilyMembers-1];
		}
		else if ( state == STATE_FIPS_HAWAII ) {
			annualPovertyGuideline = SY1PovertyGuidelinesHI[numberFamilyMembers-1];
		}
	}
	else if( month >= 7 && month <= 12 ) {
		//School Year 2
		if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
			annualPovertyGuideline = SY2PovertyGuidelinesUS[numberFamilyMembers-1];
		}
		else if ( state == STATE_FIPS_ALASKA ) {
			annualPovertyGuideline = SY2PovertyGuidelinesAK[numberFamilyMembers-1];
		}
		else if ( state == STATE_FIPS_HAWAII ) {
			annualPovertyGuideline = SY2PovertyGuidelinesHI[numberFamilyMembers-1];
		}
	}
	else {
    pHousehold->Error =	"CSchoolMealsEligible::GetPovertyGuideline: month argument out of bounds.";
		AfxThrowUserException();
	}

  monthlyPovertyGuideline = annualPovertyGuideline/12;

	return( monthlyPovertyGuideline );
}

//===================================================================
void CSchoolMealsEligible::SetEligibility() {
	//combines montetary eligibility with adjunctive eligibility information to set eligibility results variable
	//person-level processing
	bool SY1RcvdSNAP			= false;
	bool SY2RcvdSNAP			= false;
	bool SY1RcvdTANF			= false;
	bool SY2RcvdTANF			= false;
	bool SY1FreeLunch			= false;
	bool SY2FreeLunch			= false;
	bool SY1RedPriceLunch = false;
	bool SY2RedPriceLunch = false;

	for( int iMonth=0; iMonth<12; iMonth++ ) {
		switch(iMonth) {
			case 0: // School Year 1  is Jan-May
			case 1:
			case 2:
			case 3:
			case 4:
				if(MonthlyCategoricalElig[iMonth] == 1) {
					if( SY1FreeLunch || MonthlyHhMonetaryEligType[iMonth] == SM_FREE_MEALS ) {
						SY1FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( pHousehold->Person->ExpandedRelationship == 9 ) { //this is a foster child
						SY1FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( pHousehold->Person->Age < 15 && pHousehold->Person->HhFamilyRelation>=33 ) { //this is an other unrelated kid
						SY1FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( SY1RedPriceLunch || MonthlyHhMonetaryEligType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
						SY1RedPriceLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_REDUCED_PRICE_MEALS;
					}
					
					if( SY1RcvdSNAP || IsSNAPReporterUnit(iMonth) ) {
						SY1RcvdSNAP = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
						SY1FreeLunch = true;
					}
					
					if( SY1RcvdTANF || IsTANFReporterUnit(iMonth) ) {
						SY1RcvdTANF = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
						SY1FreeLunch = true;
					}

				}
				else { 
					//SMResult->MonthlyEligibilityType[iMonth] = 0;
				}
				break;
			case 8: // School Year 2 is Sep-Dec
			case 9:
			case 10:
			case 11:
				if(MonthlyCategoricalElig[iMonth] == 1) {
					if(	SY2FreeLunch ||	MonthlyHhMonetaryEligType[iMonth]	== SM_FREE_MEALS ) {
						SY2FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( pHousehold->Person->ExpandedRelationship == 9 ) { //this is a foster child
						SY2FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( pHousehold->Person->Age < 15 && pHousehold->Person->HhFamilyRelation>=33 ) { //this is an other unrelated kid
						SY2FreeLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
					}
					else if( SY2RedPriceLunch	|| MonthlyHhMonetaryEligType[iMonth] ==	SM_REDUCED_PRICE_MEALS ) {
						SY2RedPriceLunch = true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_REDUCED_PRICE_MEALS;
					}

					if( SY2RcvdSNAP ||	IsSNAPReporterUnit(iMonth) ) {
						SY2RcvdSNAP	=	true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
						SY2FreeLunch = true;
					}
					
					if( SY2RcvdTANF ||	IsTANFReporterUnit(iMonth) ) {
						SY2RcvdTANF	=	true;
						SMResult->MonthlyEligibilityType[iMonth] = SM_FREE_MEALS;
						SY2FreeLunch = true;
					}

				}
				else { 
					//SMResult->MonthlyEligibilityType[iMonth] = 0;
				}
				break;
			default:
				//currently school meals are not sumulated in other months Jun-Aug
				//Laura may want to have SNAP or TANF received in the summer confer elig -- commented out for now
				//if( IsSNAPReporterUnit(iMonth) )
				//	SY2RcvdSNAP = true;
				//	
				//if( IsTANFReporterUnit(iMonth) )
				//	SY2RcvdTANF = true;
				break;
		}
	}

	SMResult->IsEverEligFreeMeals=0;
	SMResult->IsEverEligRedPriceMeals=0;
	if( SY1FreeLunch || SY2FreeLunch ) SMResult->IsEverEligFreeMeals=1;
	if( SY1RedPriceLunch || SY2RedPriceLunch ) SMResult->IsEverEligRedPriceMeals=1;
	
}

/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsBenefit
CSchoolMealsBenefit::CSchoolMealsBenefit (CSchoolMealsSim *pParent, 
                            CHousehold *pHouse) : 
                            SMResult (pParent->Result),
                            CBenefit (pParent, pHouse) {
  pSim = pParent;
};

//========================================================================
CSchoolMealsBenefit::~CSchoolMealsBenefit() {
};

//========================================================================
void CSchoolMealsBenefit::ListNationalInst () {
  RFX_Single("MonthlySY1FreeMealBensUS", MonthlySY1FreeMealBensUS);
  RFX_Single("MonthlySY2FreeMealBensUS", MonthlySY2FreeMealBensUS);
  RFX_Single("MonthlySY1FreeMealBensAK", MonthlySY1FreeMealBensAK);
  RFX_Single("MonthlySY2FreeMealBensAK", MonthlySY2FreeMealBensAK);
  RFX_Single("MonthlySY1FreeMealBensHI", MonthlySY1FreeMealBensHI);
  RFX_Single("MonthlySY2FreeMealBensHI", MonthlySY2FreeMealBensHI);
  RFX_Single("MonthlySY1RedPriceMealBensUS", MonthlySY1RedPriceMealBensUS);
  RFX_Single("MonthlySY2RedPriceMealBensUS", MonthlySY2RedPriceMealBensUS);
  RFX_Single("MonthlySY1RedPriceMealBensAK", MonthlySY1RedPriceMealBensAK);
  RFX_Single("MonthlySY2RedPriceMealBensAK", MonthlySY2RedPriceMealBensAK);
  RFX_Single("MonthlySY1RedPriceMealBensHI", MonthlySY1RedPriceMealBensHI);
  RFX_Single("MonthlySY2RedPriceMealBensHI", MonthlySY2RedPriceMealBensHI);
};

//========================================================================
void CSchoolMealsBenefit::ListStateInst() {
}

//========================================================================
void CSchoolMealsBenefit::MoveStateFields (int StateNumber) {
}

//========================================================================
void CSchoolMealsBenefit::ListStateBracketInst () {
}

//========================================================================
void CSchoolMealsBenefit::MoveStateBracketFields (int StateNumber, int BracketNumber) {
}

//========================================================================
void CSchoolMealsBenefit::Initialize() {
  FetchInst();
}

//=========================================================================
void CSchoolMealsBenefit::Calculate() {
}

//=========================================================================
void CSchoolMealsBenefit::SetPotentialBenefits() {
	for( int iMonth=0; iMonth<12; iMonth++ ) {
		switch(iMonth) {
			case 0:  //School Year 1 Jan-May
			case 1:
			case 2:
			case 3:
			case 4:
				if( SMResult->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = GetMonthlySchoolMealSubsidies(1,SM_FREE_MEALS);
				}
				else if( SMResult->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = GetMonthlySchoolMealSubsidies(1,SM_REDUCED_PRICE_MEALS);
				}
				else {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = 0;
				}
				break;
			case 8:  //School Year 2 Sep-Dec
			case 9:
			case 10:
			case 11:
				if( SMResult->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = GetMonthlySchoolMealSubsidies(2,SM_FREE_MEALS);
				}
				else if( SMResult->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = GetMonthlySchoolMealSubsidies(2,SM_REDUCED_PRICE_MEALS);
				}
				else {
					SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = 0;
				}
				break;
			default:
				//Currently we're not simulating school meals in other months Jun-Aug
				SMResult->MonthlyEligSchoolMealSubsidies[iMonth] = 0;
				break;
		}
	}
}

//=========================================================================
float CSchoolMealsBenefit::GetMonthlySchoolMealSubsidies(int SchoolYear, int EligType) {
	float eligBenefits = 0;
	int state = pHousehold->Record.FipsStateCode;

	if( SchoolYear == 1 ) {
		//School Year 1
		if( EligType == SM_FREE_MEALS ) {
			if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY1FreeMealBensUS;
			}
			else if ( state == STATE_FIPS_ALASKA ) {
				eligBenefits = MonthlySY1FreeMealBensAK;
			}
			else if ( state == STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY1FreeMealBensHI;
			}
		}
		else if ( EligType == SM_REDUCED_PRICE_MEALS ) {
			if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY1RedPriceMealBensUS;
			}
			else if ( state == STATE_FIPS_ALASKA ) {
				eligBenefits = MonthlySY1RedPriceMealBensAK;
			}
			else if ( state == STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY1RedPriceMealBensHI;
			}
		}
		else {
			eligBenefits = 0;
		}
	}
	else if( SchoolYear == 2) {
		//School Year 2
		if( EligType == SM_FREE_MEALS ) {
			if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY2FreeMealBensUS;
			}
			else if ( state == STATE_FIPS_ALASKA ) {
				eligBenefits = MonthlySY2FreeMealBensAK;
			}
			else if ( state == STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY2FreeMealBensHI;
			}
		}
		else if ( EligType == SM_REDUCED_PRICE_MEALS ) {
			if( state != STATE_FIPS_ALASKA && state != STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY2RedPriceMealBensUS;
			}
			else if ( state == STATE_FIPS_ALASKA ) {
				eligBenefits = MonthlySY2RedPriceMealBensAK;
			}
			else if ( state == STATE_FIPS_HAWAII ) {
				eligBenefits = MonthlySY2RedPriceMealBensHI;
			}
		}
		else {
			eligBenefits = 0;
		}
	}
	else {
    pHousehold->Error =	"CSchoolMealsBenefits::GetMonthlySchoolMealSubsidies: SchoolYear out of bounds.";
		AfxThrowUserException();
	}

	return( eligBenefits );
}


/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsParticipate
CSchoolMealsParticipate::CSchoolMealsParticipate (CSchoolMealsSim *pParent, 
                            CHousehold *pHouse) : 
                            SMResult (pParent->Result),
                            CParticipate (pParent, pHouse) {
  pSim = pParent;
};

//=========================================================================
CSchoolMealsParticipate::~CSchoolMealsParticipate() {
};

//=========================================================================
void CSchoolMealsParticipate::ListNationalInst () {
	RFX_Single("ProbPartFreeMeals", ProbPartFreeMeals);
  RFX_Single("ProbPartReducedPriceMeals", ProbPartReducedPriceMeals);
};

//=========================================================================
void CSchoolMealsParticipate::ListStateInst () {
}

//=========================================================================
void CSchoolMealsParticipate::MoveStateFields (int StateNumber) {
}

//=========================================================================
void CSchoolMealsParticipate::Initialize() {
  FetchInst();

  SeedProbPartSchoolMeals =
  pHousehold->CreateRandomSequence("RandomProbSchoolMealsPart");
}

//=========================================================================
void CSchoolMealsParticipate::Calculate() {
}

//========================================================================
void CSchoolMealsParticipate::SetParticipationDecision() {
	int   highIncNoPart=0;
	float RandomNumProbParticipate  = pHousehold->GetRandom(SeedProbPartSchoolMeals);

	pUnit->GetFirstPerson();
  do {
		
	  if( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 ) {

			//set high income participation variable
			int monthsEligRedPrice = GetNumMonthsEligRedPriceMeals();
			if( SMResult->IsHighIncome==1 && SMResult->IsEverEligFreeMeals==0 && monthsEligRedPrice==1 ) 
				highIncNoPart=1;

			for( int iMonth=0; iMonth<12; iMonth++ ) {
			  if( SMResult->MonthlyEligibilityType[iMonth]==SM_FREE_MEALS ) {
					if( SMResult->RcvdFreeReducedMeals==1 || (RandomNumProbParticipate < ProbPartFreeMeals && highIncNoPart==0)) {
				    SMResult->MonthlyParticipationType[iMonth]=SM_FREE_MEALS;
						SMResult->MonthlyRcvdSchoolMealSubsidies[iMonth]=SMResult->MonthlyEligSchoolMealSubsidies[iMonth];
			    }
				}
			  else if( SMResult->MonthlyEligibilityType[iMonth]==SM_REDUCED_PRICE_MEALS ) {
					if( SMResult->RcvdFreeReducedMeals==1 || (RandomNumProbParticipate < ProbPartReducedPriceMeals  && highIncNoPart==0)) {
				    SMResult->MonthlyParticipationType[iMonth]=SM_REDUCED_PRICE_MEALS;
						SMResult->MonthlyRcvdSchoolMealSubsidies[iMonth]=SMResult->MonthlyEligSchoolMealSubsidies[iMonth];
					}
			  }
			}
		}

	} while( pUnit->GetNextPerson() );
}

//===================================================================
void CSchoolMealsParticipate::RankLikelyRecipients() {
  int  currentPerson = pUnit->GetPersonNum();
	bool allReportedMealsAssigned = FALSE;
	int  numHotMealRecipToAssign     = (int)pSim->pNumberHotMealsRecip->SumOfVar();
	int  numFreeRedMealRecipToAssign = (int)pSim->pNumberFreeRedMealsRecip->SumOfVar();

	//This first part sets a 'rank' for each child who may have received reported lunch
  pUnit->GetFirstPerson();
  do {
		//initialize some results variables for each person
		SMResult->SchoolMealsRank = 0;
    SMResult->RcvdHotMeals = 0;
		SMResult->RcvdFreeReducedMeals = 0;

	if ( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 && numHotMealRecipToAssign > 0 ) {
			if( pHousehold->Person->ExpandedRelationship==9 ) SMResult->SchoolMealsRank=1;
			else if( pHousehold->Person->HhFamilyRelation>=33 && pHousehold->Person->Age	< 15 ) SMResult->SchoolMealsRank=2;
			else if( pHousehold->Person->Age >= 6  && pHousehold->Person->Age <=15 ) SMResult->SchoolMealsRank=pHousehold->Person->Age;
			else if( pHousehold->Person->Age >= 16 && pHousehold->Person->Age <=18 
				&& pHousehold->Person->InSchoolFullOrPartTime==1 && pHousehold->Person->HsCollegeAttendance==1  ) SMResult->SchoolMealsRank=pHousehold->Person->Age;
			else if( pHousehold->Person->Age==5 ) SMResult->SchoolMealsRank=19; //50
			else if( pHousehold->Person->Age >= 16 && pHousehold->Person->Age <=18 
				&& pHousehold->Person->InSchoolFullOrPartTime==2 && pHousehold->Person->HsCollegeAttendance==1  ) SMResult->SchoolMealsRank=pHousehold->Person->Age-16+20; //Age*10
			else SMResult->SchoolMealsRank=pHousehold->Person->Age-5+23; //Age*100
			//Max rank = 36
			//Min rank = 1
    }
  } while( pUnit->GetNextPerson() );

	if(numHotMealRecipToAssign > 0 || numFreeRedMealRecipToAssign > 0) {

	  //this next part uses the 'rank' just assinged to determine which children in the house received School Meals
	  for (int rankIter=1; rankIter<=36 && (numHotMealRecipToAssign > 0 || numFreeRedMealRecipToAssign > 0); rankIter++) {
		  //we'll iterate through all of the possible ranks from min rank to max rank
      pUnit->GetFirstPerson();
      do {
		    if ( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 ) {
					if( SMResult->SchoolMealsRank==rankIter ) {
						if(numHotMealRecipToAssign > 0) {
						  SMResult->RcvdHotMeals=1;
							numHotMealRecipToAssign--;
						}
						if(numFreeRedMealRecipToAssign > 0) {
							SMResult->RcvdFreeReducedMeals=1;
							numFreeRedMealRecipToAssign--;
						}
					}
        }
      } while( pUnit->GetNextPerson() );			
	  }

	} //end if reported hot meal or free and reduced meal receipt

  pUnit->SetPerson(currentPerson);
}

int CSchoolMealsParticipate::GetNumMonthsEligRedPriceMeals() {
	int retMonthsEligRedPrice = 0;

	for( int iMonth=0; iMonth<12; iMonth++ ) {
		if( SMResult->MonthlyEligibilityType[iMonth]==SM_REDUCED_PRICE_MEALS )
			retMonthsEligRedPrice++;
	}

  return( retMonthsEligRedPrice );
}

/////////////////////////////////////////////////////////////////////////////
// TABLES

//========================================================================
/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableB1
CSchoolMealsTableB1::CSchoolMealsTableB1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
             CTable (pResultDatabase, pParent, pHouse), 
								Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 5;
	Setup (3, 1);  //First argument is number of rows including totals
  TableID = "B1";
  CTable::pSim = pParent;
  pSim = pParent;

	//initialize variables
	for(int iMonth=0; iMonth<12; iMonth++ ) {
		numEligFree[iMonth]				= 0;
		eligBensFree[iMonth]			= 0;
		numPartFree[iMonth]				= 0;
		partBensFree[iMonth]			= 0;
		numEligRedPrice[iMonth]		= 0;
		eligBensRedPrice[iMonth]	= 0;
		numPartRedPrice[iMonth]		= 0;
		partBensRedPrice[iMonth]  = 0;
	}
}

//========================================================================
void CSchoolMealsTableB1::UpdateTable() {
  float weight;
	pUnit->GetFirstPerson();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
				numEligFree[iMonth]		+= weight;
				eligBensFree[iMonth]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
			else if( Result->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				numEligRedPrice[iMonth]	+= weight;
				eligBensRedPrice[iMonth]+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}

			//participating kids
			if( Result->MonthlyParticipationType[iMonth] == SM_FREE_MEALS ) {
				numPartFree[iMonth]		+= weight;
				partBensFree[iMonth]	+= weight*Result->MonthlyRcvdSchoolMealSubsidies[iMonth];
			}
			else if( Result->MonthlyParticipationType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				numPartRedPrice[iMonth]	+= weight;
				partBensRedPrice[iMonth]+= weight*Result->MonthlyRcvdSchoolMealSubsidies[iMonth];
			}
		}
  } while (pUnit->GetNextPerson());

}

//========================================================================
void CSchoolMealsTableB1::OnWrite() {
  float avgNumEligFree			= 0;
	float avgNumPartFree			= 0;
	float totEligBensFree			= 0;
	float totPartBensFree			= 0;
  float avgNumEligRedPrice	= 0;
	float avgNumPartRedPrice	= 0;
	float totEligBensRedPrice = 0;
	float totPartBensRedPrice = 0;

	//calculate final figures for table
	avgNumEligFree = (numEligFree[0] + numEligFree[1] + numEligFree[2]	+ numEligFree[3] + numEligFree[4] 
									+ numEligFree[8] + numEligFree[9] + numEligFree[10] + numEligFree[11])/9;
	avgNumEligRedPrice = (numEligRedPrice[0] + numEligRedPrice[1] + numEligRedPrice[2]	+ numEligRedPrice[3] + numEligRedPrice[4] 
											+ numEligRedPrice[8] + numEligRedPrice[9] + numEligRedPrice[10] + numEligRedPrice[11])/9;	
	avgNumPartFree = (numPartFree[0] + numPartFree[1] + numPartFree[2]	+ numPartFree[3] + numPartFree[4] 
									+ numPartFree[8] + numPartFree[9] + numPartFree[10] + numPartFree[11])/9;
	avgNumPartRedPrice = (numPartRedPrice[0] + numPartRedPrice[1] + numPartRedPrice[2]	+ numPartRedPrice[3] + numPartRedPrice[4] 
											+ numPartRedPrice[8] + numPartRedPrice[9] + numPartRedPrice[10] + numPartRedPrice[11])/9;
	for( int iMonth=0; iMonth<12; iMonth++ ) {
		totEligBensFree			+= eligBensFree[iMonth];
		totPartBensFree			+= partBensFree[iMonth];
		totEligBensRedPrice	+= eligBensRedPrice[iMonth];
		totPartBensRedPrice	+= partBensRedPrice[iMonth];
	}

	//free meals row
  pCol[0]->pVal[0] = avgNumEligFree;
  pCol[1]->pVal[0] = avgNumPartFree;
	if( avgNumEligFree > 0 ) { //prevent divide by 0
		pCol[2]->pVal[0] = avgNumPartFree/avgNumEligFree*100;
	}
	else {
		pCol[2]->pVal[0] = 0;
	}
  pCol[3]->pVal[0] = totEligBensFree;
  pCol[4]->pVal[0] = totPartBensFree;

	//reduced-price meals row
  pCol[0]->pVal[1] = avgNumEligRedPrice;
  pCol[1]->pVal[1] = avgNumPartRedPrice;
	if( avgNumEligRedPrice > 0 ) { //prevent divide by 0
		pCol[2]->pVal[1] = avgNumPartRedPrice/avgNumEligRedPrice*100;
	}
	else {
		pCol[2]->pVal[1] = 0;
	}
  pCol[3]->pVal[1] = totEligBensRedPrice;
  pCol[4]->pVal[1] = totPartBensRedPrice;

	//total row
	pCol[0]->pVal[2] = avgNumEligFree+avgNumEligRedPrice;
  pCol[1]->pVal[2] = avgNumPartFree+avgNumPartRedPrice;
	if( (avgNumEligFree+avgNumEligRedPrice) > 0 ) { //prevent divide by 0
		pCol[2]->pVal[2] = (avgNumPartFree+avgNumPartRedPrice)/(avgNumEligFree+avgNumEligRedPrice)*100;
	}
	else {
		pCol[2]->pVal[2] = 0;
	}
  pCol[3]->pVal[2] = totEligBensFree+totEligBensRedPrice;
  pCol[4]->pVal[2] = totPartBensFree+totPartBensRedPrice;
}

//========================================================================
/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableB2
CSchoolMealsTableB2::CSchoolMealsTableB2 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
             CTable (pResultDatabase, pParent, pHouse), 
								Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 3;
	Setup (3, 1);  //First argument is number of rows including totals
  TableID = "B2";
  CTable::pSim = pParent;
  pSim = pParent;

	//initialize variables
	for(int iMonth=0; iMonth<12; iMonth++ ) {
		numEligFreePartFree[iMonth]		= 0;
	  numEligFreePartNone[iMonth]		= 0;
	  numEligRdcdPartRdcd[iMonth]		= 0;
	  numEligRdcdPartNone[iMonth]		= 0;
	  numEligNonePartNone[iMonth]		= 0;
	}
}

//========================================================================
void CSchoolMealsTableB2::UpdateTable() {
  float weight;
	pUnit->GetFirstPerson();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
				if( Result->MonthlyParticipationType[iMonth] == SM_FREE_MEALS ) {
					numEligFreePartFree[iMonth] += weight;
				}
				else numEligFreePartNone[iMonth] += weight;
			}
			else if( Result->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				if( Result->MonthlyParticipationType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
					numEligRdcdPartRdcd[iMonth] += weight;
				}
				else numEligRdcdPartNone[iMonth] += weight;
			}
			else if( pHousehold->Person->Age >= 5 && pHousehold->Person->Age <= 18 ) {
				if( pHousehold->Person->Age==5 ) {
					if( iMonth==SEPTEMBER || iMonth==OCTOBER || iMonth==NOVEMBER || iMonth==DECEMBER )
					  numEligNonePartNone[iMonth] += weight;
				}
				else if( pHousehold->Person->Age >= 16 && pHousehold->Person->HsCollegeAttendance!=1 ) {
					if( iMonth==JANUARY || iMonth==FEBRUARY || iMonth==MARCH || iMonth==APRIL || iMonth==MAY )
					  numEligNonePartNone[iMonth] += weight;
				}
				else {
					if( iMonth==JANUARY || iMonth==FEBRUARY || iMonth==MARCH || iMonth==APRIL || iMonth==MAY ||
						  iMonth==SEPTEMBER || iMonth==OCTOBER || iMonth==NOVEMBER || iMonth==DECEMBER )
					  numEligNonePartNone[iMonth] += weight;
				}
			}
		}
  } while (pUnit->GetNextPerson());

}

//========================================================================
void CSchoolMealsTableB2::OnWrite() {
  float avgNumEligFreePartFree = 0;
	float avgNumEligFreePartNone = 0;
  float avgNumEligRdcdPartRdcd = 0;
	float avgNumEligRdcdPartNone = 0;
	float avgNumEligNonePartNone = 0;

	//calculate final figures for table
	avgNumEligFreePartFree = (numEligFreePartFree[0] + numEligFreePartFree[1] + numEligFreePartFree[2]	+ numEligFreePartFree[3] + numEligFreePartFree[4] 
									        + numEligFreePartFree[8] + numEligFreePartFree[9] + numEligFreePartFree[10] + numEligFreePartFree[11])/9;
	avgNumEligFreePartNone = (numEligFreePartNone[0] + numEligFreePartNone[1] + numEligFreePartNone[2]	+ numEligFreePartNone[3] + numEligFreePartNone[4] 
											    + numEligFreePartNone[8] + numEligFreePartNone[9] + numEligFreePartNone[10] + numEligFreePartNone[11])/9;	
	avgNumEligRdcdPartRdcd = (numEligRdcdPartRdcd[0] + numEligRdcdPartRdcd[1] + numEligRdcdPartRdcd[2]	+ numEligRdcdPartRdcd[3] + numEligRdcdPartRdcd[4] 
									        + numEligRdcdPartRdcd[8] + numEligRdcdPartRdcd[9] + numEligRdcdPartRdcd[10] + numEligRdcdPartRdcd[11])/9;
	avgNumEligRdcdPartNone = (numEligRdcdPartNone[0] + numEligRdcdPartNone[1] + numEligRdcdPartNone[2]	+ numEligRdcdPartNone[3] + numEligRdcdPartNone[4] 
									        + numEligRdcdPartNone[8] + numEligRdcdPartNone[9] + numEligRdcdPartNone[10] + numEligRdcdPartNone[11])/9;
	avgNumEligNonePartNone = (numEligNonePartNone[0] + numEligNonePartNone[1] + numEligNonePartNone[2]	+ numEligNonePartNone[3] + numEligNonePartNone[4] 
									        + numEligNonePartNone[8] + numEligNonePartNone[9] + numEligNonePartNone[10] + numEligNonePartNone[11])/9;

	//free meals row
  pCol[0]->pVal[0] = avgNumEligFreePartFree;
  pCol[1]->pVal[0] = 0;
	pCol[2]->pVal[0] = 0;

	//reduced-price meals row
  pCol[0]->pVal[1] = 0;
  pCol[1]->pVal[1] = avgNumEligRdcdPartRdcd;
  pCol[2]->pVal[1] = 0;

	//none row
	pCol[0]->pVal[2] = avgNumEligFreePartNone;
  pCol[1]->pVal[2] = avgNumEligRdcdPartNone;
	pCol[2]->pVal[2] = avgNumEligNonePartNone;

}

//========================================================================
/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableA1
CSchoolMealsTableA1::CSchoolMealsTableA1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
             CTable (pResultDatabase, pParent, pHouse), 
								Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 3;
	Setup (3, 1);  //First argument is number of rows including totals
  TableID = "A1";
  CTable::pSim = pParent;
  pSim = pParent;

	//initialize variables
	for(int iMonth=0; iMonth<12; iMonth++ ) {
		numFreeReporters[iMonth]     = 0;
	  numFreeNonReporters[iMonth]  = 0;
	  numRdcdReporters[iMonth]     = 0;
	  numRdcdNonReporters[iMonth]  = 0;
	}
}

//========================================================================
void CSchoolMealsTableA1::UpdateTable() {
  float weight;
	pUnit->GetFirstPerson();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
				if( Result->RcvdFreeReducedMeals==1 ) {
					numFreeReporters[iMonth] += weight;
				}
				else numFreeNonReporters[iMonth] += weight;
			}
			else if( Result->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				if( Result->RcvdFreeReducedMeals==1 ) {
					numRdcdReporters[iMonth] += weight;
				}
				else numRdcdNonReporters[iMonth] += weight;
			}
		}
  } while (pUnit->GetNextPerson());

}

//========================================================================
void CSchoolMealsTableA1::OnWrite() {
  float avgNumFreeReporters    = 0;
	float avgNumFreeNonReporters = 0;
  float avgNumRdcdReporters    = 0;
	float avgNumRdcdNonReporters = 0;

	//calculate final figures for table
	avgNumFreeReporters    = (numFreeReporters[0] + numFreeReporters[1] + numFreeReporters[2]	+ numFreeReporters[3] + numFreeReporters[4] 
									        + numFreeReporters[8] + numFreeReporters[9] + numFreeReporters[10] + numFreeReporters[11])/9;
	avgNumFreeNonReporters = (numFreeNonReporters[0] + numFreeNonReporters[1] + numFreeNonReporters[2]	+ numFreeNonReporters[3] + numFreeNonReporters[4] 
											    + numFreeNonReporters[8] + numFreeNonReporters[9] + numFreeNonReporters[10] + numFreeNonReporters[11])/9;	
	avgNumRdcdReporters    = (numRdcdReporters[0] + numRdcdReporters[1] + numRdcdReporters[2]	+ numRdcdReporters[3] + numRdcdReporters[4] 
									        + numRdcdReporters[8] + numRdcdReporters[9] + numRdcdReporters[10] + numRdcdReporters[11])/9;
	avgNumRdcdNonReporters = (numRdcdNonReporters[0] + numRdcdNonReporters[1] + numRdcdNonReporters[2]	+ numRdcdNonReporters[3] + numRdcdNonReporters[4] 
									        + numRdcdNonReporters[8] + numRdcdNonReporters[9] + numRdcdNonReporters[10] + numRdcdNonReporters[11])/9;

	//free meals row
  pCol[0]->pVal[0] = avgNumFreeReporters;
  pCol[1]->pVal[0] = avgNumFreeNonReporters;
	pCol[2]->pVal[0] = (avgNumFreeReporters + avgNumFreeNonReporters);

	//reduced-price meals row
  pCol[0]->pVal[1] = avgNumRdcdReporters;
  pCol[1]->pVal[1] = avgNumRdcdNonReporters;
  pCol[2]->pVal[1] = (avgNumRdcdReporters + avgNumRdcdNonReporters);

	//total row
	pCol[0]->pVal[2] = (avgNumFreeReporters + avgNumRdcdReporters);
  pCol[1]->pVal[2] = (avgNumFreeNonReporters + avgNumRdcdNonReporters);
	pCol[2]->pVal[2] = (avgNumFreeReporters + avgNumRdcdReporters + avgNumFreeNonReporters + avgNumRdcdNonReporters);

}

/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableS1

CSchoolMealsTableS1::CSchoolMealsTableS1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
  CTable (pResultDatabase, pParent, pHouse), 
	Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 5;
	Setup (52, 1);  //First argument is number of rows including totals
  TableID = "S1";
  CTable::pSim = pParent;
  pSim = pParent;

	for(int stateIter=0; stateIter<52; stateIter++) {
		pCol[0]->pVal[stateIter] = 0;
		pCol[1]->pVal[stateIter] = 0;
		pCol[2]->pVal[stateIter] = 0;
		pCol[3]->pVal[stateIter] = 0;
		pCol[4]->pVal[stateIter] = 0;
	}
}

void CSchoolMealsTableS1::UpdateTable() {
	float weight;
	int   row;
	pUnit->GetFirstPerson();

	row = StateIndex();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
				pCol[0]->pVal[row]	+= weight;
				pCol[3]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
			else if( Result->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				pCol[0]->pVal[row]	+= weight;
				pCol[3]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}

			//participating kids
			if( Result->MonthlyParticipationType[iMonth] == SM_FREE_MEALS ) {
				pCol[1]->pVal[row]	+= weight;
				pCol[4]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
			else if( Result->MonthlyParticipationType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				pCol[1]->pVal[row]	+= weight;
				pCol[4]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
		}
  } while (pUnit->GetNextPerson());
	pSim->pUnit->GetFirstPerson();
}

void CSchoolMealsTableS1::OnWrite() {
	for(int stateIter=0; stateIter<51; stateIter++) {
		pCol[0]->pVal[stateIter] = pCol[0]->pVal[stateIter]/9;
		pCol[1]->pVal[stateIter] = pCol[1]->pVal[stateIter]/9;
		pCol[2]->pVal[stateIter] = (pCol[1]->pVal[stateIter]/pCol[0]->pVal[stateIter])*100;

		//total row
		pCol[0]->pVal[51] += pCol[0]->pVal[stateIter];
		pCol[1]->pVal[51] += pCol[1]->pVal[stateIter];
		pCol[3]->pVal[51] += pCol[3]->pVal[stateIter];
		pCol[4]->pVal[51] += pCol[4]->pVal[stateIter];
	}

	pCol[2]->pVal[51] = (pCol[1]->pVal[51]/pCol[0]->pVal[51])*100;
}

/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableS2

CSchoolMealsTableS2::CSchoolMealsTableS2 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
  CTable (pResultDatabase, pParent, pHouse), 
	Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 5;
	Setup (52, 1);  //First argument is number of rows including totals
  TableID = "S2";
  CTable::pSim = pParent;
  pSim = pParent;

	for(int stateIter=0; stateIter<52; stateIter++) {
		pCol[0]->pVal[stateIter] = 0;
		pCol[1]->pVal[stateIter] = 0;
		pCol[2]->pVal[stateIter] = 0;
		pCol[3]->pVal[stateIter] = 0;
		pCol[4]->pVal[stateIter] = 0;
	}
}

void CSchoolMealsTableS2::UpdateTable() {
	float weight;
	int   row;
	pUnit->GetFirstPerson();

	row = StateIndex();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_FREE_MEALS ) {
				pCol[0]->pVal[row]	+= weight;
				pCol[3]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}

			//participating kids
			if( Result->MonthlyParticipationType[iMonth] == SM_FREE_MEALS ) {
				pCol[1]->pVal[row]	+= weight;
				pCol[4]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
		}
  } while (pUnit->GetNextPerson());
	pSim->pUnit->GetFirstPerson();
}

void CSchoolMealsTableS2::OnWrite() {
	for(int stateIter=0; stateIter<51; stateIter++) {
		pCol[0]->pVal[stateIter] = pCol[0]->pVal[stateIter]/9;
		pCol[1]->pVal[stateIter] = pCol[1]->pVal[stateIter]/9;
		pCol[2]->pVal[stateIter] = (pCol[1]->pVal[stateIter]/pCol[0]->pVal[stateIter])*100;

		//total row
		pCol[0]->pVal[51] += pCol[0]->pVal[stateIter];
		pCol[1]->pVal[51] += pCol[1]->pVal[stateIter];
		pCol[3]->pVal[51] += pCol[3]->pVal[stateIter];
		pCol[4]->pVal[51] += pCol[4]->pVal[stateIter];
	}

	pCol[2]->pVal[51] = (pCol[1]->pVal[51]/pCol[0]->pVal[51])*100;
}

/////////////////////////////////////////////////////////////////////////////
// CSchoolMealsTableS3

CSchoolMealsTableS3::CSchoolMealsTableS3 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse) :
  CTable (pResultDatabase, pParent, pHouse), 
	Result(pParent->Result) // so can use Result-> rather than pSim->Result 
{
	NumIdx = 1;
	NumCol = 5;
	Setup (52, 1);  //First argument is number of rows including totals
  TableID = "S3";
  CTable::pSim = pParent;
  pSim = pParent;

	for(int stateIter=0; stateIter<52; stateIter++) {
		pCol[0]->pVal[stateIter] = 0;
		pCol[1]->pVal[stateIter] = 0;
		pCol[2]->pVal[stateIter] = 0;
		pCol[3]->pVal[stateIter] = 0;
		pCol[4]->pVal[stateIter] = 0;
	}
}

void CSchoolMealsTableS3::UpdateTable() {
	float weight;
	int   row;
	pUnit->GetFirstPerson();

	row = StateIndex();

	do {
    weight = pHousehold->Person->PersonWeight;

		for( int iMonth=0; iMonth<12; iMonth++ ) {
			//eligible kids
			if( Result->MonthlyEligibilityType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				pCol[0]->pVal[row]	+= weight;
				pCol[3]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}

			//participating kids
			if( Result->MonthlyParticipationType[iMonth] == SM_REDUCED_PRICE_MEALS ) {
				pCol[1]->pVal[row]	+= weight;
				pCol[4]->pVal[row]	+= weight*Result->MonthlyEligSchoolMealSubsidies[iMonth];
			}
		}
  } while (pUnit->GetNextPerson());
	pSim->pUnit->GetFirstPerson();
}

void CSchoolMealsTableS3::OnWrite() {
	for(int stateIter=0; stateIter<51; stateIter++) {
		pCol[0]->pVal[stateIter] = pCol[0]->pVal[stateIter]/9;
		pCol[1]->pVal[stateIter] = pCol[1]->pVal[stateIter]/9;
		pCol[2]->pVal[stateIter] = (pCol[1]->pVal[stateIter]/pCol[0]->pVal[stateIter])*100;

		//total row
		pCol[0]->pVal[51] += pCol[0]->pVal[stateIter];
		pCol[1]->pVal[51] += pCol[1]->pVal[stateIter];
		pCol[3]->pVal[51] += pCol[3]->pVal[stateIter];
		pCol[4]->pVal[51] += pCol[4]->pVal[stateIter];
	}

	pCol[2]->pVal[51] = (pCol[1]->pVal[51]/pCol[0]->pVal[51])*100;
}

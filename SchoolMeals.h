/*================================================================
Copyright(C) 2012, The Urban Institute. All Rights Reserved.

Author: Michael Martinez-Schiferl
Date: August 27, 2012
=================================================================*/

#ifndef SchoolMeals
#define SchoolMeals

#define MAX_ARRAY_SIZE 20
#define JANUARY   0
#define FEBRUARY  1 
#define MARCH     2
#define APRIL     3                     
#define MAY       4
#define JUNE      5
#define JULY      6                     
#define AUGUST    7
#define SEPTEMBER 8
#define OCTOBER   9                    
#define NOVEMBER  10
#define DECEMBER  11
#define STATE_FIPS_ALASKA 2
#define STATE_FIPS_HAWAII 15

enum SchoolMealsSimMode {
  ALTERNATE_SIM_MODE = 0,
  BASELINE_SIM_MODE  = 1,
};

enum EnumEligibilityParticipationType {
  SM_NOT_ELIGIBLE					= 0,
  SM_FREE_MEALS						= 1,
  SM_REDUCED_PRICE_MEALS  = 2
};

//class name declarations for the precompiler
class CSchoolMealsEligible;
class CSchoolMealsBenefit;
class CSchoolMealsParticipate;

//========================================================================================
class CSchoolMealsMicroResults : public CResultType 
{	
public:
	int   IsEverEligFreeMeals;
	int		IsEverEligRedPriceMeals;
	int		IsHighIncome;
	int		RcvdFreeReducedMeals;
	int		RcvdHotMeals;
	int   SchoolMealsRank;
  int		MonthlyEligibilityType[12];
  int		MonthlyParticipationType[12];
  float MonthlyEligSchoolMealSubsidies[12];
  float MonthlyRcvdSchoolMealSubsidies[12];
};

//========================================================================================
class CSchoolMealsResults : public CResultSet
{                
public:
  CSchoolMealsResults::CSchoolMealsMicroResults *pTempSchoolSimResult;

  CSchoolMealsResults (CDatabase *pResultDatabase, CSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsResults();
  void Initialize ();
private:
  CResult <CSchoolMealsMicroResults> &Result;

  virtual void CopyResultRecord (char *&p);
  void Register();
};                                   

//========================================================================================
class CSchoolMealsMonthlyResults : public CResultMonthlySet
{                
public:
  CSchoolMealsMicroResults *pTempSchoolSimResult;

  CSchoolMealsMonthlyResults (CDatabase *pResultDatabase,
                         CSim *pParent,
                         CHousehold *pHouse);
  ~CSchoolMealsMonthlyResults() {};
// Overridable
  void Initialize ();
private:
  CResult <CSchoolMealsMicroResults> &Result;
  virtual void CopyMonthRecord (int Month, char *&p);
  void Register();
};                                   

//========================================================================================
class CSchoolMealsSim : public CSSim
{
public:
  CResult<CSchoolMealsMicroResults> Result;

  CSchoolMealsSim
    (CDatabase *pResultDatabase, 
       const char *pSimulationName, 
       const char *pSimulationID,
       CHousehold *pHouse);
  ~CSchoolMealsSim();
  static CSim *CreateSim (CDatabase *pResultDatabase, 
                          const char *pSimulationName, 
                          const char *pSimulationID,
                          CHousehold *pHouse);

  //Allow access to all rules from other classes
  //National rules

  //Variable list rules
  CVarInstPackager *pMonthlyEarnings;
  CVarInstPackager *pMonthlySsiBenefits;
  CVarInstPackager *pMonthlyUnearnedIncome;
  CVarInstPackager *pMonthlyTANFBenefits;
  CVarInstPackager *pMonthlyUnitSNAPBenefits;
  CVarInstPackager *pMonthlyUnitTANFBenefits;
	CVarInstPackager *pNumberFreeRedMealsRecip;
  CVarInstPackager *pNumberHotMealsRecip;

  //Public functions

private:
  CSchoolMealsEligible    *pEligible;
  CSchoolMealsBenefit     *pBenefit;
  CSchoolMealsParticipate *pParticipate;

  // Standard TRIM Simulation Functions
  void Initialize();
  void Simulate();
  void ListNationalInst();
  void ListVarInst();
  void ListStateInst();
  void MoveStateFields (int StateNumber);
  void CreateTables (CDatabase *pDatabase);

  // Unique School Lunch Functions
  void  CheckRules();
  int   IncludeThisHousehold();

  bool 	HouseholdProcessingHasBegun;
	int SimulationMode;

  // Forms
  CFunc *pIsHouseholdIncluded;
};

//===========================================================================
class CSchoolMealsUnit : public CUnit, public CInstSet
{
public:
  CSchoolMealsUnit (CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsUnit();

  void Initialize();
  void ListStateInst ();
  void MoveStateFields (int StateNumber);
	void ListVarInst();
  BOOL GetFirstUnit();
  void AdjustUnits();
	void MergeUnits(int UnitNumToDrop, int UnitNumToMergeInto);

private:
  CResult<CSchoolMealsMicroResults> &SMResult;
	CSchoolMealsSim *pSim;
};
//==================================================================================================


//==================================================================================================
class CSchoolMealsEligible : public CEligible
{
public:
  CSchoolMealsEligible(CSchoolMealsSim *pParent, 
	                  CHousehold *pHouse);
  ~CSchoolMealsEligible();

  void ListNationalInst();
  void ListStateInst();
  void MoveStateFields(int StateNumber);
  void ListStateBracketInst();
  void MoveStateBracketFields(int StateNumber, int BracketNumber);
  void ListBracketInst();
  void MoveBracketFields(int BracketNumber);
  void ListVarInst();
  void Initialize();
  virtual void Calculate();

  bool  IsAnyoneCategoricallyEligible();
  void  SetCategoricallyEligible();
  void  SetMonetarilyEligible();
  bool  IsTANFReporterUnit(int month);
  bool  IsSNAPReporterUnit(int month);
  int   GetNumPersonsInFamily();
	float GetPovertyGuideline(int month, int numberFamilyMembers);
	void  SetEligibility();

	int		MonthlyHhMonetaryEligType[12];
	float MonthlyHhIncome[12];
	int   MonthlyCategoricalElig[12];

private:  
  CResult <CSchoolMealsMicroResults> &SMResult;
  CSchoolMealsSim *pSim;
  
  //National Rules:
	float FreeMealsPctGuidelines;
	float HighIncomePctGuidelines;
	float ReducedPriceMealsPctGuidelines;

	//National Array Rules:
	float SY1PovertyGuidelinesUS[20];
	float SY1PovertyGuidelinesAK[20];
	float SY1PovertyGuidelinesHI[20];
	float SY2PovertyGuidelinesUS[20];
	float SY2PovertyGuidelinesAK[20];
	float SY2PovertyGuidelinesHI[20];

};

//==================================================================================================


//==================================================================================================
class CSchoolMealsBenefit : public CBenefit
{
public:
  CSchoolMealsBenefit(CSchoolMealsSim *pParent, 
	              CHousehold *pHouse);
  ~CSchoolMealsBenefit();

  void ListNationalInst();
  void ListStateInst();
  void MoveStateFields(int StateNumber);
  void ListStateBracketInst();
  void MoveStateBracketFields(int StateNumber, int BracketNumber);
  void Initialize();
  virtual void Calculate();

  void	SetPotentialBenefits();
	float GetMonthlySchoolMealSubsidies(int SchoolYear, int EligType);

private:  
  CResult <CSchoolMealsMicroResults> &SMResult;
  CSchoolMealsSim *pSim;

  //National Rules:
  float MonthlySY1FreeMealBensUS;
  float MonthlySY2FreeMealBensUS;
  float MonthlySY1FreeMealBensAK;
  float MonthlySY2FreeMealBensAK;
  float MonthlySY1FreeMealBensHI;
  float MonthlySY2FreeMealBensHI;
  float MonthlySY1RedPriceMealBensUS;
  float MonthlySY2RedPriceMealBensUS;
  float MonthlySY1RedPriceMealBensAK;
  float MonthlySY2RedPriceMealBensAK;
  float MonthlySY1RedPriceMealBensHI;
  float MonthlySY2RedPriceMealBensHI;
};
//==================================================================================================

//==================================================================================================
class CSchoolMealsParticipate : public CParticipate
{
public:
  CSchoolMealsParticipate(CSchoolMealsSim *pParent, 
	                  CHousehold *pHouse);
  ~CSchoolMealsParticipate();

  void ListNationalInst();
  void ListStateInst();
  void MoveStateFields(int StateNumber);
  void Initialize();
  virtual void Calculate();

  void SetParticipationDecision();
	void RankLikelyRecipients();
  int GetNumMonthsEligRedPriceMeals();

private:  
  CResult <CSchoolMealsMicroResults> &SMResult;
  CSchoolMealsSim *pSim;

  //National Rules:
	float ProbPartFreeMeals;
  float ProbPartReducedPriceMeals;

  RandomSequence SeedProbPartSchoolMeals;
};

/*=============================================================
TABLES
*/

///////////////////////////////////////////////////////////////
// CSchoolMealsTableB1

class CSchoolMealsTableB1 : public CTable
{

public:
  CSchoolMealsTableB1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableB1() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

	float numEligFree[12];
  float eligBensFree[12];
	float numPartFree[12];
	float partBensFree[12];
	float numEligRedPrice[12];
  float eligBensRedPrice[12];
	float numPartRedPrice[12];
	float partBensRedPrice[12];
};

///////////////////////////////////////////////////////////////
// CSchoolMealsTableB2

class CSchoolMealsTableB2 : public CTable
{

public:
  CSchoolMealsTableB2 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableB2() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

	float numEligFreePartFree[12];
	float numEligFreePartNone[12];
	float numEligRdcdPartRdcd[12];
	float numEligRdcdPartNone[12];
	float numEligNonePartNone[12];
};

///////////////////////////////////////////////////////////////
// CSchoolMealsTableA1

class CSchoolMealsTableA1 : public CTable
{

public:
  CSchoolMealsTableA1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableA1() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

	float numFreeReporters[12];
	float numFreeNonReporters[12];
	float numRdcdReporters[12];
	float numRdcdNonReporters[12];
};

///////////////////////////////////////////////////////////////
// CSchoolMealsTableS1

class CSchoolMealsTableS1 : public CTable
{

public:
  CSchoolMealsTableS1 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableS1() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

};

///////////////////////////////////////////////////////////////
// CSchoolMealsTableS2

class CSchoolMealsTableS2 : public CTable
{

public:
  CSchoolMealsTableS2 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableS2() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

};

///////////////////////////////////////////////////////////////
// CSchoolMealsTableS3

class CSchoolMealsTableS3 : public CTable
{

public:
  CSchoolMealsTableS3 (CDatabase *pResultDatabase, CSchoolMealsSim *pParent, CHousehold *pHouse);
  ~CSchoolMealsTableS3() {};
  void UpdateTable();
  void OnWrite();
  int row;
  int col;

private:
	CResult<CSchoolMealsMicroResults> &Result;
	CSchoolMealsSim *pSim;

};

#endif
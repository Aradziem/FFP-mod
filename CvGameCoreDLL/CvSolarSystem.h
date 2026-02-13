//Header file for CvSolarSystem.cpp
//note that there are 2 classes

//quite useful statement
#pragma once

class CvPlanet;
class CvSolarSystem;

#include "CvDLLEntity.h"

class CvPlanet  //defines a planet
{
public:
	CvPlanet(int iX = INVALID_PLOT_COORD, int iY = INVALID_PLOT_COORD , PlanetTypes iPlanetType = NO_PLANETTYPE, PlanetSizes iPlanetSize = NO_PLANETSIZE, int iOrbitRing = 0, bool bMoon = false, bool bRings = false);
	~CvPlanet();
	void reset(int iX = INVALID_PLOT_COORD, int iY = INVALID_PLOT_COORD,PlanetTypes iPlanetType = NO_PLANETTYPE, PlanetSizes iPlanetSize = NO_PLANETSIZE, int iOrbitRing = 0, bool bMoon = false, bool bRings = false);

	CvWString getName();
	void setName(CvWString szName);

	int getX() const;
	int getY() const;
	PlanetTypes getPlanetType() const;
	PlanetSizes getPlanetSize() const;
	int getOrbitRing() const;

	bool isMoon() const;
	bool isRings() const;

	bool isPlanetWithinCulturalRange() const;
	int getPlanetCulturalRange() const;

	int getPopulation() const;
	void setPopulation(int iValue);
	void changePopulation(int iChange);
	int getPopulationLimit(PlayerTypes iOwner);

	bool isDisabled() const;
	void setDisabled(bool bValue);

	void setBonusType(int iBonusType);
	int getBonusType() const;
	bool isBonus() const;
	bool isHasBonus(int iBonusType) const;

	bool isHasBuilding(int iID);
	void setHasBuilding(int iID, bool bValue);
	int getBuildingProduction(int iID);
	void setBuildingProduction(int iID, int iValue);

	int getTotalYield(int iOwner, YieldTypes iYieldID);
	int getBaseYield(YieldTypes iYieldID) const;
	int getExtraYield(int iOwner, YieldTypes iYieldID);

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);
protected:
	CvWString m_szName;

	int m_iX;
	int m_iY;

	PlanetTypes m_iPlanetType;
	PlanetSizes m_iPlanetSize;
	int m_iOrbitRing;
	bool m_bMoon;
	bool m_bRings;

	int m_iPopulation;

	bool m_bDisabled;

	int m_iBonus;

	bool* m_abHasBuilding;
	int* m_aiBuildingProduction;
};

class CvSolarSystem : public CvDLLEntity  //defines a solar system
{
public:
	CvSolarSystem(int iX = -1, int iY = -1, bool bIsStarbase = false, StarTypes iSunType = NO_STARTYPE);
	virtual ~CvSolarSystem();
	void reset(int iX = -1, int iY = -1, bool bIsStarbase = false, StarTypes iSunType = NO_STARTYPE);
	//void setupGraphical();

	int getID() const;
	void setID(int newID);

	bool isNeedsUpdate() const;
	void setNeedsUpdate(bool bValue);

	void setCity(IDInfo pCity);
	bool setCity();

	void updateYield();

	PlayerTypes getOwner() const;
	CvCity* getCity() const;
	int getPopulation();
	int getX() const;
	int getY() const;

	bool getIsStarbase() const;
	void setIsStarbase(bool bNewValue);

	StarTypes getStarType() const;
	void setStarType(StarTypes iStarType);

	int getSelectedPlanet() const;
	void setSelectedPlanet(int iID);

	int getBuildingPlanetRing() const;
	void setBuildingPlanetRing(int iID);
	void setSingleBuildingRingLocation(int iBuildingType);
	int getSingleBuildingRingLocationByType(int iBuildingType);

	int** getSingleBuildingLocations();
	int getNumPlanets() const;
	CvPlanet* getPlanetByIndex(int iID);
	int getPlanetIndexByRing(int ring);

	void addPlanet(int iPlanetType = -1, int iPlanetSize = 0, int iOrbitRing = 0, int bMoon = false, int bRings = false);
	CvPlanet* getPlanet(int iRingID);

	int getPopulationLimit(bool bFactorHappiness = false);

	int** getSizeYieldPlanetIndexList(YieldTypes iYield);

	void addBasicBuildingsToBestPlanet();
	int getBestPlanetInSystem();
	
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);
	//	def getData(self):;
	//	def setData(self, aArray):;
	void updateDisplay();

	

protected:
	int m_ID;

	bool m_bNeedsUpdate;

	int m_iX;
	int m_iY;

	IDInfo m_pCity;

	bool m_bIsStarbase;

	StarTypes m_iStarType;

	int m_iSelectedPlanet;
	int m_iBuildingPlanetRing;

	int	m_iSingleBuildingLocations;//length of m_aaiSingleBuildingLocations
	int** m_aaiSingleBuildingLocations;//type: int[m_iSingleBuildingLocations][2]

	int m_iNumPlanets;
	CvPlanet** m_apPlanets;

	int* m_piYields;
};



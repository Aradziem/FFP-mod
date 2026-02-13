#pragma once
#ifndef CySolarSystem_h
#define CySolarSystem_h

//Aradziem
class CyPlanet;
class CySolarSystem
{
public:
	CySolarSystem();
	CySolarSystem(CvSolarSystem* pSolarSystem);
	int getSolarSystemID();
	int getSolarSystemX();
	int getSolarSystemY();

	bool getSolarSystemNeedsUpdate();
	void setSolarSystemNeedsUpdate(bool newValue);
	void updateSolarSystemDisplay();
	int getSolarSystemNumPlanets();

	int getBuildingPlanetID();
	int getBuildingPlanetRing();
	void setBuildingPlanetRing(int Ring);
	int getSelectedPlanetRing();
	int getSelectedPlanetID();
	void setSelectedPlanetRing(int Ring);
	void setSelectedPlanetID(int ID);
	int getPlanetIDByRing(int planetRing);

	int getPopulationLimit(bool bFactorHappiness);
	

	CyPlanet* getPlanet(int iID);
private:
	CvSolarSystem* m_pSolarSystem;
};
class CyPlanet
{
public:
	CyPlanet();
	CyPlanet(CvPlanet* pPlanet);
	
	int getPlanetRing();
	int getPlanetSize();
	int getPlanetType();
	int getPlanetBonus();
	bool isPlanetBonus();
	std::wstring getPlanetName();
	int getPlanetYield(int iOwner, int bonusType);
	int getPlanetPopulation();
	int getPlanetPopulationLimit(int iOwner);
	bool isPlanetDisabled();
	bool isPlanetRings();
	bool isPlanetMoon();
	bool isPlanetBuilding(int buildingID);
	bool isPlanetWithinCulturalRange();
	int getPlanetCulturalRange();
private:
	CvPlanet* m_pPlanet;
};

#endif	// CySolarSystem_h

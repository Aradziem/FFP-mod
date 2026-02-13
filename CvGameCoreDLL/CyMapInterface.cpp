#include "CvGameCoreDLL.h"
#include "CyMap.h"
#include "CyArea.h"
#include "CyCity.h"
#include "CySelectionGroup.h"
#include "CyUnit.h"
#include "CyPlot.h"
#include "CySolarSystem.h"
//#include "CvStructs.h"
//# include <boost/python/manage_new_object.hpp>
//# include <boost/python/return_value_policy.hpp>

//
// published python interface for CyMap
//

void CyMapPythonInterface()
{
	OutputDebugString("Python Extension Module - CyMapPythonInterface\n");

	python::class_<CyMap>("CyMap")
		.def("isNone", &CyMap::isNone, "bool () - valid CyMap() interface")

		.def("erasePlots", &CyMap::erasePlots, "() - erases the plots")
		.def("setRevealedPlots", &CyMap::setRevealedPlots, "void (int /*TeamTypes*/ eTeam, bool bNewValue, bool bTerrainOnly) - reveals the plots to eTeam")
		.def("setAllPlotTypes", &CyMap::setAllPlotTypes, "void (int /*PlotTypes*/ ePlotType) - sets all plots to ePlotType")

		.def("updateVisibility", &CyMap::updateVisibility, "() - updates the plots visibility")
		.def("syncRandPlot", &CyMap::syncRandPlot, python::return_value_policy<python::manage_new_object>(), "CyPlot* (iFlags,iArea,iMinUnitDistance,iTimeout) - random plot based on conditions")
		.def("findCity", &CyMap::findCity, python::return_value_policy<python::manage_new_object>(), "CyCity* (int iX, int iY, int (PlayerTypes) eOwner = NO_PLAYER, int (TeamTypes) eTeam = NO_TEAM, bool bSameArea = true, bool bCoastalOnly = false, int (TeamTypes) eTeamAtWarWith = NO_TEAM, int (DirectionTypes) eDirection = NO_DIRECTION, CvCity* pSkipCity = NULL) - finds city")
		.def("findSelectionGroup", &CyMap::findSelectionGroup, python::return_value_policy<python::manage_new_object>(), "CvSelectionGroup* (int iX, int iY, int /*PlayerTypes*/ eOwner, bool bReadyToSelect, bool bWorkers)")

		.def("findBiggestArea", &CyMap::findBiggestArea, python::return_value_policy<python::manage_new_object>(), "CyArea* ()")

		.def("getMapFractalFlags", &CyMap::getMapFractalFlags, "int ()")
		.def("findWater", &CyMap::findWater, "bool (CyPlot* pPlot, int iRange, bool bFreshWater)")
		.def("isPlot", &CyMap::isPlot, "bool (iX,iY) - is (iX, iY) a valid plot?")
		.def("numPlots", &CyMap::numPlots, "int () - total plots in the map")
		.def("plotNum", &CyMap::plotNum, "int (iX,iY) - the index for a given plot") 
		.def("plotX", &CyMap::plotX, "int (iIndex) - given the index of a plot, returns its X coordinate") 
		.def("plotY", &CyMap::plotY, "int (iIndex) - given the index of a plot, returns its Y coordinate") 
		.def("getGridWidth", &CyMap::getGridWidth, "int () - the width of the map, in plots")
		.def("getGridHeight", &CyMap::getGridHeight, "int () - the height of the map, in plots")

		.def("getLandPlots", &CyMap::getLandPlots, "int () - total land plots")
		.def("getOwnedPlots", &CyMap::getOwnedPlots, "int () - total owned plots")

		.def("getTopLatitude", &CyMap::getTopLatitude, "int () - top latitude (usually 90)")
		.def("getBottomLatitude", &CyMap::getBottomLatitude, "int () - bottom latitude (usually -90)")

		.def("getNextRiverID", &CyMap::getNextRiverID, "int ()")
		.def("incrementNextRiverID", &CyMap::incrementNextRiverID, "void ()")

		.def("isWrapX", &CyMap::isWrapX, "bool () - whether the map wraps in the X axis")
		.def("isWrapY", &CyMap::isWrapY, "bool () - whether the map wraps in the Y axis")
		.def("getMapScriptName", &CyMap::getMapScriptName, "wstring () - name of the map script")
		.def("getWorldSize", &CyMap::getWorldSize, "WorldSizeTypes () - size of the world")
		.def("getClimate", &CyMap::getClimate, "ClimateTypes () - climate of the world")
		.def("getSeaLevel", &CyMap::getSeaLevel, "SeaLevelTypes () - sealevel of the world")

		.def("getNumCustomMapOptions", &CyMap::getNumCustomMapOptions, "int () - number of custom map settings")
		.def("getCustomMapOption", &CyMap::getCustomMapOption, "CustomMapOptionTypes () - user defined map setting at this option id")

		.def("getNumBonuses", &CyMap::getNumBonuses, "int () - total bonuses")
		.def("getNumBonusesOnLand", &CyMap::getNumBonusesOnLand, "int () - total bonuses on land plots")

		.def("plotByIndex", &CyMap::plotByIndex, python::return_value_policy<python::manage_new_object>(), "CyPlot (iIndex) - get a plot by its Index")
		.def("sPlotByIndex", &CyMap::sPlotByIndex, python::return_value_policy<python::reference_existing_object>(), "CyPlot (iIndex) - static - get plot by iIndex")
		.def("plot", &CyMap::plot, python::return_value_policy<python::manage_new_object>(), "CyPlot (iX,iY) - get CyPlot at (iX,iY)")
		.def("sPlot", &CyMap::sPlot, python::return_value_policy<python::reference_existing_object>(), "CyPlot (iX,iY) - static - get CyPlot at (iX,iY)")
		.def("pointToPlot", &CyMap::pointToPlot, python::return_value_policy<python::manage_new_object>())
		.def("getIndexAfterLastArea", &CyMap::getIndexAfterLastArea, "int () - index for handling NULL areas")
		.def("getNumAreas", &CyMap::getNumAreas, "int () - total areas")
		.def("getNumLandAreas", &CyMap::getNumLandAreas, "int () - total land areas")
		.def("getArea", &CyMap::getArea, python::return_value_policy<python::manage_new_object>(), "CyArea (iID) - get CyArea at iID")
		.def("recalculateAreas", &CyMap::recalculateAreas, "void () - Recalculates the areaID for each plot. Should be preceded by CyMap.setPlotTypes(...)")
		.def("resetPathDistance", &CyMap::resetPathDistance, "void ()")

		.def("calculatePathDistance", &CyMap::calculatePathDistance, "finds the shortest passable path between two CyPlots and returns its length, or returns -1 if no such path exists. Note: the path must be all-land or all-water")
		.def("rebuild", &CyMap::rebuild, "used to initialize the map during WorldBuilder load")
		.def("regenerateGameElements", &CyMap::regenerateGameElements, "used to regenerate everything but the terrain and height maps")
		.def("updateFog", &CyMap::updateFog, "void ()")
		.def("updateMinimapColor", &CyMap::updateMinimapColor, "void ()")
		.def("updateMinOriginalStartDist", &CyMap::updateMinOriginalStartDist, "void (CyArea* pArea)")

		//Aradziem
		.def("generateSolarSystems", &CyMap::generateSolarSystems, "void ()")
		.def("getNumSystems", &CyMap::getNumSolarSystems, "int ()")
		.def("resetSolarSystems", &CyMap::resetSolarSystems, "void ()")

		.def("getSolarSystem", &CyMap::getSystem,python::return_value_policy<python::manage_new_object>(),"CySolarSystem* (int ID)")
		.def("getSolarSystemID", &CyMap::getSolarSystemID, "int (int x,int y)")
		/*.def("getSolarSystemX", &CyMap::getSolarSystemX, "int (int ID)")
		.def("getSolarSystemY", &CyMap::getSolarSystemY, "int (int ID)")
		.def("getSolarSystemNeedsUpdate", &CyMap::getSolarSystemNeedsUpdate, "int (int ID)")
		.def("setSolarSystemNeedsUpdate", &CyMap::setSolarSystemNeedsUpdate, "void (int ID, bool newValue)")
		.def("updateSolarSystemDisplay", &CyMap::updateSolarSystemDisplay, "void (int ID)")
		.def("getSolarSystemNumPlanets", &CyMap::getSolarSystemNumPlanets, "int (int ID)")
		.def("getBuildingPlanetRing", &CyMap::getBuildingPlanetRing, "int (int ID)")
		.def("setBuildingPlanetRing", &CyMap::setBuildingPlanetRing, "void (int ID,int Ring)")
		.def("getSelectedPlanetRing", &CyMap::getSelectedPlanetRing, "int (int ID)")
		.def("setSelectedPlanetRing", &CyMap::setSelectedPlanetRing, "void (int ID,int Ring)")

		.def("getPlanetIDByRing", &CyMap::getPlanetIDByRing, "int (int solarSystemID, int ring)")
		.def("getPlanetRing",&CyMap::getPlanetRing,"int (int solarSystemID, int ID)")
		.def("getPlanetSize",&CyMap::getPlanetSize,"int (int solarSystemID, int ID)")
		.def("getPlanetType", &CyMap::getPlanetType, "int (int solarSystemID, int ID)")
		.def("getPlanetBonus", &CyMap::getPlanetBonus, "int (int solarSystemID, int ID)")
		.def("isPlanetBonus", &CyMap::isPlanetBonus, "bool (int solarSystemID, int ID)")
		.def("getPlanetName", &CyMap::getPlanetName, "std::wstring (int solarSystemID, int ID)")
		.def("getPlanetYield", &CyMap::getPlanetYield, "int (int solarSystemID, int ID , int iOwner, int yieldType)")
		.def("getPlanetPopulation", &CyMap::getPlanetPopulation, "int (int solarSystemID, int ID)")
		.def("isPlanetDisabled", &CyMap::isPlanetDisabled, "bool (int solarSystemID, int ID)")
		.def("isPlanetRings", &CyMap::isPlanetRings, "bool (int solarSystemID, int ID)")
		.def("isPlanetMoon", &CyMap::isPlanetMoon, "bool (int solarSystemID, int ID)")
		.def("isPlanetBuilding", &CyMap::isPlanetBuilding, "bool (int solarSystemID, int ID, int buildingID)")
		.def("getPlanetCulturalRange", &CyMap::getPlanetCulturalRange, "int (int solarSystemID, int ID)")*/
		;

	python::class_<CySolarSystem>("CySolarSystem")
		.def("getID", &CySolarSystem::getSolarSystemID, "int ()")
		.def("getX", &CySolarSystem::getSolarSystemX, "int ()")
		.def("getY", &CySolarSystem::getSolarSystemY, "int ()")
		.def("getNeedsUpdate", &CySolarSystem::getSolarSystemNeedsUpdate, "int ()")
		.def("setNeedsUpdate", &CySolarSystem::setSolarSystemNeedsUpdate, "void ( bool newValue)")
		.def("updateDisplay", &CySolarSystem::updateSolarSystemDisplay, "void ()")
		.def("getNumPlanets", &CySolarSystem::getSolarSystemNumPlanets, "int ()")
		.def("getBuildingPlanetID", &CySolarSystem::getBuildingPlanetID, "int ()")
		.def("getBuildingPlanetRing", &CySolarSystem::getBuildingPlanetRing, "int ()")
		.def("setBuildingPlanetRing", &CySolarSystem::setBuildingPlanetRing, "void (int Ring)")
		.def("getSelectedPlanetRing", &CySolarSystem::getSelectedPlanetRing, "int ()")
		.def("getSelectedPlanetID", &CySolarSystem::getSelectedPlanetID, "int ()")
		.def("setSelectedPlanetID", &CySolarSystem::setSelectedPlanetID, "void (int ID)")
		.def("setSelectedPlanetRing", &CySolarSystem::setSelectedPlanetRing, "void (int Ring)")
		.def("getPlanetIDByRing", &CySolarSystem::getPlanetIDByRing, "int (int ring)")
		.def("getPopulationLimit", &CySolarSystem::getPopulationLimit, "int (bool bFactorHappiness)")
		.def("getPlanet", &CySolarSystem::getPlanet, python::return_value_policy<python::manage_new_object>(), "CyPlanet* (int ID)")
		
		;
	python::class_<CyPlanet>("CyPlanet")
		.def("getRing", &CyPlanet::getPlanetRing, "int ()")
		.def("getSize", &CyPlanet::getPlanetSize, "int ()")
		.def("getType", &CyPlanet::getPlanetType, "int ()")
		.def("getBonus", &CyPlanet::getPlanetBonus, "int ()")
		.def("isBonus", &CyPlanet::isPlanetBonus, "bool ()")
		.def("getName", &CyPlanet::getPlanetName, "std::wstring ()")
		.def("getYield", &CyPlanet::getPlanetYield, "int (int iOwner, int yieldType)")
		.def("getPopulation", &CyPlanet::getPlanetPopulation, "int ()")
		.def("getPopulationLimit", &CyPlanet::getPlanetPopulationLimit, "int (int iOwner)")
		.def("isDisabled", &CyPlanet::isPlanetDisabled, "bool ()")
		.def("isRings", &CyPlanet::isPlanetRings, "bool ()")
		.def("isMoon", &CyPlanet::isPlanetMoon, "bool ()")
		.def("isBuilding", &CyPlanet::isPlanetBuilding, "bool (int buildingID)")
		.def("getCulturalRange", &CyPlanet::getPlanetCulturalRange, "int ()")
		.def("isWithinCulturalRange", &CyPlanet::isPlanetWithinCulturalRange, "bool ()")
		;
	//Aradziem end
}

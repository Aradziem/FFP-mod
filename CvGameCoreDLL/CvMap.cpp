//
//	FILE:	 CvMap.cpp
//	AUTHOR:  Soren Johnson
//	PURPOSE: Game map class
//-----------------------------------------------------------------------------
//	Copyright (c) 2004 Firaxis Games, Inc. All rights reserved.
//-----------------------------------------------------------------------------
//


#include "CvGameCoreDLL.h"
#include "CvMap.h"
#include "CvCity.h"
#include "CvPlotGroup.h"
#include "CvGlobals.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGameCoreUtils.h"
#include "CvFractal.h"
#include "CvPlot.h"
#include "CvMapGenerator.h"
#include "FAStarNode.h"
#include "CvInitCore.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "CyArgsList.h"

#include "CvDLLEngineIFaceBase.h"
#include "CvDLLIniParserIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h" // K-Mod
#include "CvSolarSystem.h"

// Public Functions...

CvMap::CvMap()
{
	CvMapInitData defaultMapData;

	m_paiNumBonus = NULL;
	m_paiNumBonusOnLand = NULL;

	m_pMapPlots = NULL;

	reset(&defaultMapData);

	//these don't have to be reset or saved
	//memcpy(m_aaiPlanetYields, aaiPlanetYields, sizeof(m_aaiPlanetYields));

	m_i_RandomPlanetNames0 = 46;
	m_i_RandomPlanetNames1 = 41;
	m_i_RandomPlanetNames2 = 22;
	m_i_RandomPlanetNames3 = 24;
	for (int i = 0;i < m_i_RandomPlanetNames0; i++)
	{
		m_g_aszRandomPlanetNames0[i] = (wchar*)"TXT_KEY_RANDOM_PLANET_0_" + CvWString::format(L"%d", i);
	}
	for (int i = 0;i < m_i_RandomPlanetNames1; i++)
	{
		m_g_aszRandomPlanetNames1[i] = (wchar*)"TXT_KEY_RANDOM_PLANET_1_" + CvWString::format(L"%d", i);
	}
	for (int i = 0;i < m_i_RandomPlanetNames2; i++)
	{
		m_g_aszRandomPlanetNames2[i] = (wchar*)"TXT_KEY_RANDOM_PLANET_2_" + CvWString::format(L"%d", i);
	}
	for (int i = 0;i < m_i_RandomPlanetNames3; i++)
	{
		m_g_aszRandomPlanetNames3[i] = (wchar*)"TXT_KEY_RANDOM_PLANET_3_" + CvWString::format(L"%d", i);
	}

	m_g_iPlanetRange2 = 3;
	m_g_iPlanetRange3 = 5;
}


CvMap::~CvMap()
{
	uninit();
	SAFE_DELETE_ARRAY(m_aaiPlanetYields);

}

// FUNCTION: init()
// Initializes the map.
// Parameters:
//	pInitInfo					- Optional init structure (used for WB load)
// Returns:
//	nothing.
void CvMap::init(CvMapInitData* pInitInfo/*=NULL*/)
{
	int iX, iY;

	PROFILE("CvMap::init");
	gDLL->logMemState( CvString::format("CvMap::init begin - world size=%s, climate=%s, sealevel=%s, num custom options=%6", 
		GC.getWorldInfo(GC.getInitCore().getWorldSize()).getDescription(), 
		GC.getClimateInfo(GC.getInitCore().getClimate()).getDescription(), 
		GC.getSeaLevelInfo(GC.getInitCore().getSeaLevel()).getDescription(),
		GC.getInitCore().getNumCustomMapOptions()).c_str() );

	gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "beforeInit");

	//--------------------------------
	// Init saved data
	reset(pInitInfo);

	//--------------------------------
	// Init containers
	i_numSolarSystemsSlots = 1;
	m_solarSystems = new CvSolarSystem*[1];
	m_solarSystems[0] = NULL;
	m_areas.init();

	//--------------------------------
	// Init non-saved data
	setup();

	//--------------------------------
	// Init other game data
	gDLL->logMemState("CvMap before init plots");
	m_pMapPlots = new CvPlot[numPlotsINLINE()];
	for (iX = 0; iX < getGridWidthINLINE(); iX++)
	{
		gDLL->callUpdater();
		for (iY = 0; iY < getGridHeightINLINE(); iY++)
		{
			plotSorenINLINE(iX, iY)->init(iX, iY);
		}
	}
	calculateAreas();
	gDLL->logMemState("CvMap after init plots");
}


void CvMap::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNumBonus);
	SAFE_DELETE_ARRAY(m_paiNumBonusOnLand);

	SAFE_DELETE_ARRAY(m_pMapPlots);

	m_areas.uninit();
	SAFE_DELETE_ARRAY(m_solarSystems);
}

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvMap::reset(CvMapInitData* pInitInfo)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	//
	// set grid size
	// initially set in terrain cell units
	//
	m_iGridWidth = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ? GC.getWorldInfo(GC.getInitCore().getWorldSize()).getGridWidth (): 0;	//todotw:tcells wide
	m_iGridHeight = (GC.getInitCore().getWorldSize() != NO_WORLDSIZE) ? GC.getWorldInfo(GC.getInitCore().getWorldSize()).getGridHeight (): 0;

	// allow grid size override
	if (pInitInfo)
	{
		m_iGridWidth	= pInitInfo->m_iGridW;
		m_iGridHeight	= pInitInfo->m_iGridH;
	}
	else
	{
		// check map script for grid size override
		if (GC.getInitCore().getWorldSize() != NO_WORLDSIZE)
		{
			std::vector<int> out;
			CyArgsList argsList;
			argsList.add(GC.getInitCore().getWorldSize());
			bool ok = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getGridSize", argsList.makeFunctionArgs(), &out);

			if (ok && !gDLL->getPythonIFace()->pythonUsingDefaultImpl() && out.size() == 2)
			{
				m_iGridWidth = out[0];
				m_iGridHeight = out[1];
				FAssertMsg(m_iGridWidth > 0 && m_iGridHeight > 0, "the width and height returned by python getGridSize() must be positive");
			}
		}

		// convert to plot dimensions
		if (GC.getNumLandscapeInfos() > 0)
		{
			m_iGridWidth *= GC.getLandscapeInfo(GC.getActiveLandscapeID()).getPlotsPerCellX();
			m_iGridHeight *= GC.getLandscapeInfo(GC.getActiveLandscapeID()).getPlotsPerCellY();
		}
	}

	m_iLandPlots = 0;
	m_iOwnedPlots = 0;

	if (pInitInfo)
	{
		m_iTopLatitude = pInitInfo->m_iTopLatitude;
		m_iBottomLatitude = pInitInfo->m_iBottomLatitude;
	}
	else
	{
		// Check map script for latitude override (map script beats ini file)

		long resultTop = -1, resultBottom = -1;
		bool okX = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getTopLatitude", NULL, &resultTop);
		bool overrideX = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();
		bool okY = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getBottomLatitude", NULL, &resultBottom);
		bool overrideY = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();

		if (okX && okY && overrideX && overrideY && resultTop != -1 && resultBottom != -1)
		{
			m_iTopLatitude = resultTop;
			m_iBottomLatitude = resultBottom;
		}
	}

	m_iTopLatitude = std::min(m_iTopLatitude, 90);
	m_iTopLatitude = std::max(m_iTopLatitude, -90);
	m_iBottomLatitude = std::min(m_iBottomLatitude, 90);
	m_iBottomLatitude = std::max(m_iBottomLatitude, -90);

	m_iNextRiverID = 0;

	//
	// set wrapping
	//
	m_bWrapX = true;
	m_bWrapY = false;
	if (pInitInfo)
	{
		m_bWrapX = pInitInfo->m_bWrapX;
		m_bWrapY = pInitInfo->m_bWrapY;
	}
	else
	{
		// Check map script for wrap override (map script beats ini file)

		long resultX = -1, resultY = -1;
		bool okX = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getWrapX", NULL, &resultX);
		bool overrideX = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();
		bool okY = gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "getWrapY", NULL, &resultY);
		bool overrideY = !gDLL->getPythonIFace()->pythonUsingDefaultImpl();

		if (okX && okY && overrideX && overrideY && resultX != -1 && resultY != -1)
		{
			m_bWrapX = (resultX != 0);
			m_bWrapY = (resultY != 0);
		}
	}

	if (GC.getNumBonusInfos())
	{
		FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvMap::reset");
		FAssertMsg(m_paiNumBonus==NULL, "mem leak m_paiNumBonus");
		m_paiNumBonus = new int[GC.getNumBonusInfos()];
		FAssertMsg(m_paiNumBonusOnLand==NULL, "mem leak m_paiNumBonusOnLand");
		m_paiNumBonusOnLand = new int[GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNumBonus[iI] = 0;
			m_paiNumBonusOnLand[iI] = 0;
		}
	}

	m_areas.removeAll();
	i_numSolarSystemsSlots = 0;
	m_solarSystems = NULL;
}


// FUNCTION: setup()
// Initializes all data that is not serialized but needs to be initialized after loading.
void CvMap::setup()
{
	PROFILE("CvMap::setup");

	KmodPathFinder::InitHeuristicWeights(); // K-Mod
	gDLL->getFAStarIFace()->Initialize(&GC.getPathFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), pathDestValid, pathHeuristic, pathCost, pathValid, pathAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getInterfacePathFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), pathDestValid, pathHeuristic, pathCost, pathValid, pathAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getStepFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), stepDestValid, stepHeuristic, stepCost, stepValid, stepAdd, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getRouteFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), NULL, NULL, NULL, routeValid, NULL, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getBorderFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), NULL, NULL, NULL, borderValid, NULL, NULL, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getAreaFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), NULL, NULL, NULL, areaValid, NULL, joinArea, NULL);
	gDLL->getFAStarIFace()->Initialize(&GC.getPlotGroupFinder(), getGridWidthINLINE(), getGridHeightINLINE(), isWrapXINLINE(), isWrapYINLINE(), NULL, NULL, NULL, plotGroupValid, NULL, countPlotGroup, NULL);
}


//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvMap::setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
		return;

	if (m_pMapPlots != NULL)
	{
		int iI;
		for (iI = 0; iI < numPlotsINLINE(); iI++)
		{
			gDLL->callUpdater();	// allow windows msgs to update
			plotByIndexINLINE(iI)->setupGraphical();
		}
	}
}


void CvMap::erasePlots()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->erase();
	}
}


void CvMap::setRevealedPlots(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly)
{
	PROFILE_FUNC();

	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->setRevealed(eTeam, bNewValue, bTerrainOnly, NO_TEAM, false);
	}

	GC.getGameINLINE().updatePlotGroups();
}


void CvMap::setAllPlotTypes(PlotTypes ePlotType)
{
	//float startTime = (float) timeGetTime();

	for(int i=0;i<numPlotsINLINE();i++)
	{
		plotByIndexINLINE(i)->setPlotType(ePlotType, false, false);
	}

	recalculateAreas();

	//rebuild landscape
	gDLL->getEngineIFace()->RebuildAllPlots();

	//mark minimap as dirty
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	
	//float endTime = (float) timeGetTime();
	//OutputDebugString(CvString::format("[Jason] setAllPlotTypes: %f\n", endTime - startTime).c_str());
}


// XXX generalize these funcs? (macro?)
void CvMap::doTurn()
{
	PROFILE("CvMap::doTurn()")

	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->doTurn();
	}
}

// K-Mod
void CvMap::setFlagsDirty()
{
	for (int i = 0; i < numPlotsINLINE(); i++)
	{
		plotByIndexINLINE(i)->setFlagDirty(true);
	}
}
// K-Mod end

void CvMap::updateFlagSymbols()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		pLoopPlot = plotByIndexINLINE(iI);

		if (pLoopPlot->isFlagDirty())
		{
			pLoopPlot->updateFlagSymbol();
			pLoopPlot->setFlagDirty(false);
		}
	}
}


void CvMap::updateFog()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateFog();
	}
}


void CvMap::updateVisibility()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateVisibility();
	}
}


void CvMap::updateSymbolVisibility()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateSymbolVisibility();
	}
}


void CvMap::updateSymbols()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateSymbols();
	}
}


void CvMap::updateMinimapColor()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateMinimapColor();
	}
}


void CvMap::updateSight(bool bIncrement)
{
	for (int iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateSight(bIncrement, false);
	}

	GC.getGameINLINE().updatePlotGroups();
}


void CvMap::updateIrrigated()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateIrrigated();
	}
}


// K-Mod. This function is called when the unit selection is changed, or when a selected unit is promoted. (Or when UnitInfo_DIRTY_BIT is set.)
// The purpose is to update which unit is displayed in the center of each plot.

// The original implementation simply updated every plot on the map. This is a bad idea because it scales badly for big maps, and the update function on each plot can be expensive.
// The new functionality attempts to only update plots that are in movement range of the selected group; with a very generous approximation for what might be in range.
void CvMap::updateCenterUnit()
{
	/* original bts code
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateCenterUnit();
	} */
	PROFILE_FUNC();
	int iRange = -1;

	CLLNode<IDInfo>* pSelectionNode = gDLL->getInterfaceIFace()->headSelectionListNode();
	while (pSelectionNode)
	{
		const CvUnit* pLoopUnit = ::getUnit(pSelectionNode->m_data);
		pSelectionNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectionNode);

		int iLoopRange;
		if (pLoopUnit->getDomainType() == DOMAIN_AIR)
		{
			iLoopRange = pLoopUnit->airRange();
		}
		else
		{
			int iStepCost = pLoopUnit->getDomainType() == DOMAIN_LAND ? KmodPathFinder::MinimumStepCost(pLoopUnit->baseMoves()) : GC.getMOVE_DENOMINATOR();
			iLoopRange = pLoopUnit->maxMoves() / iStepCost + (pLoopUnit->canParadrop(pLoopUnit->plot()) ? pLoopUnit->getDropRange() : 0);
		}
		iRange = std::max(iRange, iLoopRange);
		// Note: technically we only really need the minimum range; but I'm using the maximum range because I think it will produce more intuitive and useful information for the player.
	}

	if (iRange < 0 || iRange*iRange > numPlotsINLINE() / 2)
	{
		// update the whole map
		for (int i = 0; i < numPlotsINLINE(); i++)
		{
			plotByIndexINLINE(i)->updateCenterUnit();
		}
	}
	else
	{
		// only update within the range
		CvPlot* pCenterPlot = gDLL->getInterfaceIFace()->getHeadSelectedUnit()->plot();
		for (int x = -iRange; x <= iRange; x++)
		{
			for (int y = -iRange; y <= iRange; y++)
			{
				CvPlot* pLoopPlot = plotXY(pCenterPlot, x, y);
				if (pLoopPlot)
					pLoopPlot->updateCenterUnit();
			}
		}
	}
}
// K-Mod end

void CvMap::updateWorkingCity()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateWorkingCity();
	}
}


void CvMap::updateMinOriginalStartDist(CvArea* pArea)
{
	PROFILE_FUNC();

	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	int iDist;
	int iI, iJ;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		pLoopPlot = plotByIndexINLINE(iI);

		if (pLoopPlot->area() == pArea)
		{
			pLoopPlot->setMinOriginalStartDist(-1);
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

		if (pStartingPlot != NULL)
		{
			if (pStartingPlot->area() == pArea)
			{
				for (iJ = 0; iJ < numPlotsINLINE(); iJ++)
				{
					pLoopPlot = plotByIndexINLINE(iJ);

					//if (pLoopPlot->area() == pArea)
					if (pLoopPlot->area() == pArea && pLoopPlot != pStartingPlot) // K-Mod
					{
						
						//iDist = GC.getMapINLINE().calculatePathDistance(pStartingPlot, pLoopPlot);
						iDist = stepDistance(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());

						if (iDist != -1)
						{
						    //int iCrowDistance = plotDistance(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
						    //iDist = std::min(iDist,  iCrowDistance * 2);
							if ((pLoopPlot->getMinOriginalStartDist() == -1) || (iDist < pLoopPlot->getMinOriginalStartDist()))
							{
								pLoopPlot->setMinOriginalStartDist(iDist);
							}
						}
					}
				}
			}
		}
	}
}


void CvMap::updateYield()
{
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->updateYield();
	}
}


void CvMap::verifyUnitValidPlot()
{
	for (int iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->verifyUnitValidPlot();
	}
}


void CvMap::combinePlotGroups(PlayerTypes ePlayer, CvPlotGroup* pPlotGroup1, CvPlotGroup* pPlotGroup2)
{
	CLLNode<XYCoords>* pPlotNode;
	CvPlotGroup* pNewPlotGroup;
	CvPlotGroup* pOldPlotGroup;
	CvPlot* pPlot;

	FAssertMsg(pPlotGroup1 != NULL, "pPlotGroup is not assigned to a valid value");
	FAssertMsg(pPlotGroup2 != NULL, "pPlotGroup is not assigned to a valid value");

	if (pPlotGroup1 == pPlotGroup2)
	{
		return;
	}

	if (pPlotGroup1->getLengthPlots() > pPlotGroup2->getLengthPlots())
	{
		pNewPlotGroup = pPlotGroup1;
		pOldPlotGroup = pPlotGroup2;
	}
	else
	{
		pNewPlotGroup = pPlotGroup2;
		pOldPlotGroup = pPlotGroup1;
	}

	pPlotNode = pOldPlotGroup->headPlotsNode();
	while (pPlotNode != NULL)
	{
		pPlot = plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY);
		pNewPlotGroup->addPlot(pPlot);
		pPlotNode = pOldPlotGroup->deletePlotsNode(pPlotNode);
	}
}

CvPlot* CvMap::syncRandPlot(int iFlags, int iArea, int iMinUnitDistance, int iTimeout)
{
	CvPlot* pPlot;
	CvPlot* pTestPlot;
	CvPlot* pLoopPlot;
	bool bValid;
	int iCount;
	int iDX, iDY;

	pPlot = NULL;

	iCount = 0;

	while (iCount < iTimeout)
	{
		pTestPlot = plotSorenINLINE(GC.getGameINLINE().getSorenRandNum(getGridWidthINLINE(), "Rand Plot Width"), GC.getGameINLINE().getSorenRandNum(getGridHeightINLINE(), "Rand Plot Height"));

		FAssertMsg(pTestPlot != NULL, "TestPlot is not assigned a valid value");

		if ((iArea == -1) || (pTestPlot->getArea() == iArea))
		{
			bValid = true;

			if (bValid)
			{
				if (iMinUnitDistance != -1)
				{
					for (iDX = -(iMinUnitDistance); iDX <= iMinUnitDistance; iDX++)
					{
						for (iDY = -(iMinUnitDistance); iDY <= iMinUnitDistance; iDY++)
						{
							pLoopPlot	= plotXY(pTestPlot->getX_INLINE(), pTestPlot->getY_INLINE(), iDX, iDY);

							if (pLoopPlot != NULL)
							{
								if (pLoopPlot->isUnit())
								{
									bValid = false;
								}
							}
						}
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_LAND)
				{
					if (pTestPlot->isWater())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_UNOWNED)
				{
					if (pTestPlot->isOwned())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_ADJACENT_UNOWNED)
				{
					if (pTestPlot->isAdjacentOwned())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				//if (iFlags & RANDPLOT_ADJACENT_LAND)
				if ((iFlags & RANDPLOT_ADJACENT_LAND) || (iFlags & RANDPLOT_ADJACENT_LAND_FOOD_WEIGHTED)) // DarkLunaPhantom
				{
					if (!(pTestPlot->isAdjacentToLand()))
					{
						bValid = false;
					}
                    // DarkLunaPhantom begin - Adds 1 / (max(1, 2 * iMaxFood)) chance of plot being rejected. See CvGame::createBarbarianUnits. Idea from AdvCiv by f1rpo (advc.300), but implementation and details different.
                    else
                    {
                        if(iFlags & RANDPLOT_ADJACENT_LAND_FOOD_WEIGHTED)
                        {
                            int iMaxFood = 0; // DarkLunaPhantom - Maximum food yield among 3x3 land tiles around pTestPlot (if pTestPlot itself yields any food).
                            if(pTestPlot->getYield(YIELD_FOOD) > 0)
                            {
                                for(int iDX = -1; iDX <= 1; iDX++)
                                {
                                    for(int iDY = -1; iDY <= 1; iDY++)
                                    {
                                        pLoopPlot = plotXY(pTestPlot->getX_INLINE(), pTestPlot->getY_INLINE(), iDX, iDY);
                                        if(pLoopPlot != NULL && !pLoopPlot->isWater())
                                        {
                                            iMaxFood = std::max(iMaxFood, pLoopPlot->getYield(YIELD_FOOD));
                                        }
                                    }
                                }
                            }
                            if(iMaxFood == 0 || GC.getGameINLINE().getSorenRandNum(100, "RandPlot food weight") < 100 / (2 * iMaxFood))
                            {
                                bValid = false;
                            }
                        }
                    }
                    // DarkLunaPhantom end
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_PASSIBLE)
				{
					if (pTestPlot->isImpassable())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_NOT_VISIBLE_TO_CIV)
				{
					if (pTestPlot->isVisibleToCivTeam())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				if (iFlags & RANDPLOT_NOT_CITY)
				{
					if (pTestPlot->isCity())
					{
						bValid = false;
					}
				}
			}

			if (bValid)
			{
				pPlot = pTestPlot;
				break;
			}
		}

		iCount++;
	}

	return pPlot;
}


CvCity* CvMap::findCity(int iX, int iY, PlayerTypes eOwner, TeamTypes eTeam, bool bSameArea, bool bCoastalOnly, TeamTypes eTeamAtWarWith, DirectionTypes eDirection, CvCity* pSkipCity)
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	// XXX look for barbarian cities???

	iBestValue = MAX_INT;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				if ((eTeam == NO_TEAM) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
				{
					for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
					{
						if (!bSameArea || (pLoopCity->area() == plotINLINE(iX, iY)->area()) || (bCoastalOnly && (pLoopCity->waterArea() == plotINLINE(iX, iY)->area())))
						{
							if (!bCoastalOnly || pLoopCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
							{
								if ((eTeamAtWarWith == NO_TEAM) || atWar(GET_PLAYER((PlayerTypes)iI).getTeam(), eTeamAtWarWith))
								{
									if ((eDirection == NO_DIRECTION) || (estimateDirection(dxWrap(pLoopCity->getX_INLINE() - iX), dyWrap(pLoopCity->getY_INLINE() - iY)) == eDirection))
									{
										if ((pSkipCity == NULL) || (pLoopCity != pSkipCity))
										{
											iValue = plotDistance(iX, iY, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

											if (iValue < iBestValue)
											{
												iBestValue = iValue;
												pBestCity = pLoopCity;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return pBestCity;
}


CvSelectionGroup* CvMap::findSelectionGroup(int iX, int iY, PlayerTypes eOwner, bool bReadyToSelect, bool bWorkers)
{
	CvSelectionGroup* pLoopSelectionGroup;
	CvSelectionGroup* pBestSelectionGroup;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	// XXX look for barbarian cities???

	iBestValue = MAX_INT;
	pBestSelectionGroup = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((eOwner == NO_PLAYER) || (iI == eOwner))
			{
				for(pLoopSelectionGroup = GET_PLAYER((PlayerTypes)iI).firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = GET_PLAYER((PlayerTypes)iI).nextSelectionGroup(&iLoop))
				{
					if (!bReadyToSelect || pLoopSelectionGroup->readyToSelect())
					{
						if (!bWorkers || pLoopSelectionGroup->hasWorker())
						{
							iValue = plotDistance(iX, iY, pLoopSelectionGroup->getX(), pLoopSelectionGroup->getY());

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestSelectionGroup = pLoopSelectionGroup;
							}
						}
					}
				}
			}
		}
	}

	return pBestSelectionGroup;
}


CvArea* CvMap::findBiggestArea(bool bWater)
{
	CvArea* pLoopArea;
	CvArea* pBestArea;
	int iValue;
	int iBestValue;
	int iLoop;

	iBestValue = 0;
	pBestArea = NULL;

	for(pLoopArea = firstArea(&iLoop); pLoopArea != NULL; pLoopArea = nextArea(&iLoop))
	{
		if (pLoopArea->isWater() == bWater)
		{
			iValue = pLoopArea->getNumTiles();

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestArea = pLoopArea;
			}
		}
	}

	return pBestArea;
}


int CvMap::getMapFractalFlags()
{
	int wrapX = 0;
	if (isWrapXINLINE())
	{
		wrapX = (int)CvFractal::FRAC_WRAP_X;
	}

	int wrapY = 0;
	if (isWrapYINLINE())
	{
		wrapY = (int)CvFractal::FRAC_WRAP_Y;
	}

	return (wrapX | wrapY);
}


//"Check plots for wetlands or seaWater.  Returns true if found"
bool CvMap::findWater(CvPlot* pPlot, int iRange, bool bFreshWater)
{
	PROFILE("CvMap::findWater()");

	CvPlot* pLoopPlot;
	int iDX, iDY;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (bFreshWater)
				{
					if (pLoopPlot->isFreshWater())
					{
						return true;
					}
				}
				else
				{
					if (pLoopPlot->isWater())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


bool CvMap::isPlot(int iX, int iY) const
{
	return isPlotINLINE(iX, iY);
}


int CvMap::numPlots() const																											 
{
	return numPlotsINLINE();
}


int CvMap::plotNum(int iX, int iY) const
{
	return plotNumINLINE(iX, iY);
}


int CvMap::plotX(int iIndex) const
{
	return (iIndex % getGridWidthINLINE());
}


int CvMap::plotY(int iIndex) const
{
	return (iIndex / getGridWidthINLINE());
}


int CvMap::pointXToPlotX(float fX)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fX + (fWidth/2.0f)) / fWidth) * getGridWidthINLINE());
}


float CvMap::plotXToPointX(int iX)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iX * fWidth) / ((float)getGridWidthINLINE())) - (fWidth / 2.0f) + (GC.getPLOT_SIZE() / 2.0f);
}


int CvMap::pointYToPlotY(float fY)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return (int)(((fY + (fHeight/2.0f)) / fHeight) * getGridHeightINLINE());
}


float CvMap::plotYToPointY(int iY)
{
	float fWidth, fHeight;
	gDLL->getEngineIFace()->GetLandscapeGameDimensions(fWidth, fHeight);
	return ((iY * fHeight) / ((float)getGridHeightINLINE())) - (fHeight / 2.0f) + (GC.getPLOT_SIZE() / 2.0f);
}


float CvMap::getWidthCoords()																	
{
	return (GC.getPLOT_SIZE() * ((float)getGridWidthINLINE()));
}


float CvMap::getHeightCoords()																	
{
	return (GC.getPLOT_SIZE() * ((float)getGridHeightINLINE()));
}

/* DarkLunaPhantom begin - Idea from Tides of War by SevenSpirits. Explanation by SevenSpirits:
"Distance maintenance is inversely proportional to the longest distance possible on the map, with distance defined as max(xdistance, ydistance) + 1/2 * min(xdistance, ydistance). When a map wraps in a dimension,
the longest possible distance in that dimension is N / 2; if it doesn't wrap, the longest possible distance is N - 1. Therefore, more wrapping => shorter longest distance => higher distance maintenance. And therefore,
toroidal maps have high maintenance, flat maps have low maintenance, and cylindrical maps have medium maintenance."
SevenSpirits changed the calculation so that it always assumes that there is x wrap and no y wrap. I changed it so that it assumes that there is a wrap along the longer side, and no wrap along the shorter side.
*/
//int CvMap::maxPlotDistance()
int CvMap::maxPlotDistance(bool bDefaultWrap)
{
    if (bDefaultWrap)
    {
        return std::min(std::max(1, plotDistance(0, 0, getGridWidthINLINE() / 2, getGridHeightINLINE() - 1)), std::max(1, plotDistance(0, 0, getGridWidthINLINE() - 1, getGridHeightINLINE() / 2)));
    }
// DarkLunaPhantom end
	return std::max(1, plotDistance(0, 0, ((isWrapXINLINE()) ? (getGridWidthINLINE() / 2) : (getGridWidthINLINE() - 1)), ((isWrapYINLINE()) ? (getGridHeightINLINE() / 2) : (getGridHeightINLINE() - 1))));
}


int CvMap::maxStepDistance()
{
	return std::max(1, stepDistance(0, 0, ((isWrapXINLINE()) ? (getGridWidthINLINE() / 2) : (getGridWidthINLINE() - 1)), ((isWrapYINLINE()) ? (getGridHeightINLINE() / 2) : (getGridHeightINLINE() - 1))));
}


int CvMap::getGridWidth() const
{
	return getGridWidthINLINE();
}


int CvMap::getGridHeight() const
{
	return getGridHeightINLINE();
}


int CvMap::getLandPlots()
{
	return m_iLandPlots;
}


void CvMap::changeLandPlots(int iChange)
{
	m_iLandPlots = (m_iLandPlots + iChange);
	FAssert(getLandPlots() >= 0);
}


int CvMap::getOwnedPlots()
{
	return m_iOwnedPlots;
}


void CvMap::changeOwnedPlots(int iChange)
{
	m_iOwnedPlots = (m_iOwnedPlots + iChange);
	FAssert(getOwnedPlots() >= 0);
}


int CvMap::getTopLatitude()
{
	return m_iTopLatitude;
}


int CvMap::getBottomLatitude()
{
	return m_iBottomLatitude;
}


int CvMap::getNextRiverID()
{
	return m_iNextRiverID;
}


void CvMap::incrementNextRiverID()
{
	m_iNextRiverID++;
}


bool CvMap::isWrapX()
{
	return isWrapXINLINE();
}


bool CvMap::isWrapY()
{
	return isWrapYINLINE();
}

bool CvMap::isWrap()
{
	return isWrapINLINE();
}

WorldSizeTypes CvMap::getWorldSize()
{
	return GC.getInitCore().getWorldSize();
}


ClimateTypes CvMap::getClimate()
{
	return GC.getInitCore().getClimate();
}


SeaLevelTypes CvMap::getSeaLevel()
{
	return GC.getInitCore().getSeaLevel();
}



int CvMap::getNumCustomMapOptions()
{
	return GC.getInitCore().getNumCustomMapOptions();
}


CustomMapOptionTypes CvMap::getCustomMapOption(int iOption)
{
	return GC.getInitCore().getCustomMapOption(iOption);
}


int CvMap::getNumBonuses(BonusTypes eIndex)													
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonus[eIndex];
}


void CvMap::changeNumBonuses(BonusTypes eIndex, int iChange)									
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiNumBonus[eIndex] = (m_paiNumBonus[eIndex] + iChange);
	FAssert(getNumBonuses(eIndex) >= 0);
}


int CvMap::getNumBonusesOnLand(BonusTypes eIndex)													
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonusOnLand[eIndex];
}


void CvMap::changeNumBonusesOnLand(BonusTypes eIndex, int iChange)									
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiNumBonusOnLand[eIndex] = (m_paiNumBonusOnLand[eIndex] + iChange);
	FAssert(getNumBonusesOnLand(eIndex) >= 0);
}


CvPlot* CvMap::plotByIndex(int iIndex) const
{
	return plotByIndexINLINE(iIndex);
}


CvPlot* CvMap::plot(int iX, int iY)const
{
	return plotINLINE(iX, iY);
}


CvPlot* CvMap::pointToPlot(float fX, float fY)													
{
	return plotINLINE(pointXToPlotX(fX), pointYToPlotY(fY));
}


int CvMap::getIndexAfterLastArea()																
{
	return m_areas.getIndexAfterLast();
}


int CvMap::getNumAreas()																		
{
	return m_areas.getCount();
}


int CvMap::getNumLandAreas()
{
	CvArea* pLoopArea;
	int iNumLandAreas;
	int iLoop;

	iNumLandAreas = 0;

	for(pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoop))
	{
		if (!(pLoopArea->isWater()))
		{
			iNumLandAreas++;
		}
	}

	return iNumLandAreas;
}


CvArea* CvMap::getArea(int iID)																
{
	return m_areas.getAt(iID);
}


CvArea* CvMap::addArea()
{
	return m_areas.add();
}


void CvMap::deleteArea(int iID)
{
	m_areas.removeAt(iID);
}


CvArea* CvMap::firstArea(int *pIterIdx, bool bRev)
{
	return !bRev ? m_areas.beginIter(pIterIdx) : m_areas.endIter(pIterIdx);
}


CvArea* CvMap::nextArea(int *pIterIdx, bool bRev)
{
	return !bRev ? m_areas.nextIter(pIterIdx) : m_areas.prevIter(pIterIdx);
}


void CvMap::recalculateAreas()
{
	PROFILE("CvMap::recalculateAreas");

	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		plotByIndexINLINE(iI)->setArea(FFreeList::INVALID_INDEX);
	}

	m_areas.removeAll();

	calculateAreas();
}
//Aradziem
CvSolarSystem* CvMap::getSolarSystem(int iID)
{
	return m_solarSystems[iID];
}

CvSolarSystem* CvMap::getSolarSystemAt(int iX, int  iY)
{
	for (short iL = 0;iL < i_numSolarSystemsSlots;iL++)
	{
		if (m_solarSystems[iL] != NULL)
		{
			if ((m_solarSystems[iL]->getY() == iY && m_solarSystems[iL]->getX() == iX))
			{
				return m_solarSystems[iL];
			}
		}
	}
	return NULL;
}

void CvMap::addSolarSystem(CvSolarSystem* pSystem)
{
	if (m_solarSystems[pSystem->getID()] == NULL)
	{
		m_solarSystems[pSystem->getID()]->~CvSolarSystem();
	}

	m_solarSystems[pSystem->getID()]= pSystem;
}

int CvMap::getFreeSolarSystemIndex()
{
	for (int i = 0; i < i_numSolarSystemsSlots;i++)
	{
		if (m_solarSystems[i] == NULL)
		{
			return i;
		}
	}
	return -1;
}

void CvMap::growSolarSystemsTable()
{
	int oldSolarSysytemsNum = i_numSolarSystemsSlots;
	i_numSolarSystemsSlots*=2;
	CvSolarSystem** aSystemsTemp= new CvSolarSystem * [i_numSolarSystemsSlots];
	for (int i = 0; i < oldSolarSysytemsNum;i++)
	{
		aSystemsTemp[i] = m_solarSystems[i];
	}
	for (int i = oldSolarSysytemsNum; i<i_numSolarSystemsSlots;i++)
	{
		aSystemsTemp[i] = NULL;
	}
	m_solarSystems = aSystemsTemp;
}

void CvMap::setSolarSystem(CvSolarSystem* system, int iID)
{
	
	if (iID==-1)
	{
		iID = getFreeSolarSystemIndex();
		if (iID ==-1)
		{
			growSolarSystemsTable();
			iID = getFreeSolarSystemIndex();
		}

	}
	m_solarSystems[iID] = system;
	system->setID(iID);
}


void CvMap::deleteSolarSystem(int iID)
{
	m_solarSystems[iID]->~CvSolarSystem();
	m_solarSystems[iID] = NULL;
}

void CvMap::resetSystems()
{
	SAFE_DELETE_ARRAY( m_solarSystems);
}
//Aradziem end

void CvMap::resetPathDistance()
{
	gDLL->getFAStarIFace()->ForceReset(&GC.getStepFinder());
}


int CvMap::calculatePathDistance(CvPlot *pSource, CvPlot *pDest)
{
	FAStarNode* pNode;

	if (pSource == NULL || pDest == NULL)
	{
		return -1;
	}

	if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(), pSource->getX_INLINE(), pSource->getY_INLINE(), pDest->getX_INLINE(), pDest->getY_INLINE(), false, 0, true))
	{
		pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());

		if (pNode != NULL)
		{
			return pNode->m_iData1;
		}
	}

	return -1; // no passable path exists
}


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/21/09                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
// Plot danger cache
void CvMap::invalidateActivePlayerSafeRangeCache()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot)
			pLoopPlot->setActivePlayerSafeRangeCache(-1);
	}
}


void CvMap::invalidateBorderDangerCache(TeamTypes eTeam)
{
	PROFILE_FUNC();

	int iI;
	CvPlot* pLoopPlot;

	for( iI = 0; iI < numPlotsINLINE(); iI++ )
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if( pLoopPlot != NULL )
		{
			pLoopPlot->setBorderDangerCache(eTeam, false);
		}
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


//
// read object from a stream
// used during load
//
void CvMap::read(FDataStreamBase* pStream)
{
	CvMapInitData defaultMapData;

	// Init data before load
	reset(&defaultMapData);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iGridWidth);
	pStream->Read(&m_iGridHeight);
	pStream->Read(&m_iLandPlots);
	pStream->Read(&m_iOwnedPlots);
	pStream->Read(&m_iTopLatitude);
	pStream->Read(&m_iBottomLatitude);
	pStream->Read(&m_iNextRiverID);

	pStream->Read(&m_bWrapX);
	pStream->Read(&m_bWrapY);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated");
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonus);
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonusOnLand);

	if (numPlotsINLINE() > 0)
	{
		m_pMapPlots = new CvPlot[numPlotsINLINE()];
		int iI;
		for (iI = 0; iI < numPlotsINLINE(); iI++)
		{
			m_pMapPlots[iI].read(pStream);
		}
	}

	// call the read of the free list CvArea class allocations
	ReadStreamableFFreeListTrashArray(m_areas, pStream);
	//Aradziem
	pStream->Read(&i_numSolarSystemsSlots);
	bool t;
	m_solarSystems = new CvSolarSystem*[i_numSolarSystemsSlots];
	for (int i = 0; i< i_numSolarSystemsSlots;i++)
	{
		pStream->Read(&t);
		if (t)
		{
			m_solarSystems[i] = new CvSolarSystem();
			m_solarSystems[i]->read(pStream);
		}
	}
	
	//Aradziem end
	setup();
}

// save object to a stream
// used during save
//
void CvMap::write(FDataStreamBase* pStream)
{
	
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iGridWidth);
	pStream->Write(m_iGridHeight);
	pStream->Write(m_iLandPlots);
	pStream->Write(m_iOwnedPlots);
	pStream->Write(m_iTopLatitude);
	pStream->Write(m_iBottomLatitude);
	pStream->Write(m_iNextRiverID);

	pStream->Write(m_bWrapX);
	pStream->Write(m_bWrapY);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated");
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonus);
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonusOnLand);

	int iI;	
	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		m_pMapPlots[iI].write(pStream);
	}

	// call the read of the free list CvArea class allocations
	WriteStreamableFFreeListTrashArray(m_areas, pStream);
	//Aradziem
	pStream->Write(&i_numSolarSystemsSlots);
	for (int i = 0; i < i_numSolarSystemsSlots;i++)
	{
		if (m_solarSystems[i]==NULL)
		{
			pStream->Write(false);
		}
		else
		{
			pStream->Write(true);
			m_solarSystems[i]->write(pStream);
		}
	}
	//Aradziem end
	//FAssertMsg(false,"writing");
}


//
// used for loading WB maps
//
void CvMap::rebuild(int iGridW, int iGridH, int iTopLatitude, int iBottomLatitude, bool bWrapX, bool bWrapY, WorldSizeTypes eWorldSize, ClimateTypes eClimate, SeaLevelTypes eSeaLevel, int iNumCustomMapOptions, CustomMapOptionTypes * aeCustomMapOptions)
{
	CvMapInitData initData(iGridW, iGridH, iTopLatitude, iBottomLatitude, bWrapX, bWrapY);

	// Set init core data
	GC.getInitCore().setWorldSize(eWorldSize);
	GC.getInitCore().setClimate(eClimate);
	GC.getInitCore().setSeaLevel(eSeaLevel);
	GC.getInitCore().setCustomMapOptions(iNumCustomMapOptions, aeCustomMapOptions);

	// Init map
	init(&initData);
}
//Aradziem
//globals access - moved from globals becaue they caused an error
int CvMap::getRandomPlanetNames0() const
{
	return m_i_RandomPlanetNames0;
}
int CvMap::getRandomPlanetNames1() const
{
	return m_i_RandomPlanetNames1;
}
int CvMap::getRandomPlanetNames2() const
{
	return m_i_RandomPlanetNames1;
}
int CvMap::getRandomPlanetNames3() const
{
	return m_i_RandomPlanetNames1;
}
CvWString* CvMap::getStringRandomPlanetNames0()
{
	return m_g_aszRandomPlanetNames0;
}
CvWString* CvMap::getStringRandomPlanetNames1()
{
	return m_g_aszRandomPlanetNames1;
}
CvWString* CvMap::getStringRandomPlanetNames2()
{
	return m_g_aszRandomPlanetNames2;
}
CvWString* CvMap::getStringRandomPlanetNames3()
{
	return m_g_aszRandomPlanetNames3;
}

/*
*
	def getData(self):

		aArray = []

		aArray.append(self.getName())							# 0
		aArray.append(self.getX())								# 1
		aArray.append(self.getY())								# 2
		aArray.append(self.getPlanetType())					# 3
		aArray.append(self.getPlanetSize())					# 4
		aArray.append(self.getOrbitRing())						# 5
		aArray.append(self.isMoon())								# 6
		aArray.append(self.isRings())								# 7
		aArray.append(self.getPopulation())					# 8
		aArray.append(self.isDisabled())					# 9
		aArray.append(self.getBonusType())						# 10
		#printd("Saving out Population: %d" %(self.getPopulation()))
		for iBuildingLoop in range(gc.getNumBuildingInfos()):
			aArray.append(self.isHasBuilding(iBuildingLoop))	# 11 through whatever
		for iBuildingLoop in range(gc.getNumBuildingInfos()):
			aArray.append(self.getBuildingProduction(iBuildingLoop))	# ? through whatever

		return aArray

	def setData(self, aArray):

		iIterator = 0

		self.szName				= aArray[iIterator]	# 0
		iIterator += 1
		self.iX					= aArray[iIterator]	# 1
		iIterator += 1
		self.iY					= aArray[iIterator]	# 2
		iIterator += 1
		self.iPlanetType		= aArray[iIterator]	# 3
		iIterator += 1
		self.iPlanetSize		= aArray[iIterator]	# 4
		iIterator += 1
		self.iOrbitRing			= aArray[iIterator]	# 5
		iIterator += 1
		self.bMoon				= aArray[iIterator]	# 6
		iIterator += 1
		self.bRings				= aArray[iIterator]	# 7
		iIterator += 1
		self.iPopulation		= aArray[iIterator]	# 8
		iIterator += 1
		self.bDisabled			= aArray[iIterator]	# 9
		iIterator += 1
		self.iBonus				= aArray[iIterator]	# 10
		iIterator += 1
		for iBuildingLoop in range(gc.getNumBuildingInfos()):
			self.abHasBuilding[iBuildingLoop] = aArray[iIterator]	# 11 through whatever
			iIterator += 1
		for iBuildingLoop in range(gc.getNumBuildingInfos()):
			self.aiBuildingProduction[iBuildingLoop] = aArray[iIterator]	# ? through whatever
			iIterator += 1
*/


bool CvMap::isPlotAPlayersStart(CvPlot* pPlot)
{

	for (int iPlayerLoop = 0;iPlayerLoop < GC.getMAX_CIV_PLAYERS();iPlayerLoop++)
	{
		PlayerTypes pPlayer = (PlayerTypes)iPlayerLoop;

		if (GET_PLAYER(pPlayer).isAlive())
		{

			CvPlot* pStartPlot = GET_PLAYER(pPlayer).getStartingPlot();

			if (pStartPlot->getX() == pPlot->getX() && pStartPlot->getY() == pPlot->getY())
			{
				return true;
			}
		}
	}
	return false;
}
/*
######################################################
#		Returns an array of randomized orbit ring locations based on the number of planets this system has
######################################################
*/

int** CvMap::getRandomizedPlanetInfo(int iNumPlanets, YieldTypes iPreferredYield)
{

	int* aiPlanetRingsArray = getRandomizedOrbitRingArray(iNumPlanets);

	int** aaiPlanetInfos = new int* [iNumPlanets];

	//# Loop through all planets to add
	for (int iPlanetLoop = 0; iPlanetLoop < iNumPlanets; iPlanetLoop++)
	{
		int iOrbitRing = aiPlanetRingsArray[iPlanetLoop];
		PlanetSizes iSize = NO_PLANETSIZE;
		bool bMoon = false;
		bool bRings = false;

		int iSizeRoll = GC.getGameINLINE().getSorenRandNum(GC.getNumPlanetSizeInfos(), "Size of planet random roll");
		iSize = (PlanetSizes)iSizeRoll;

		int iMoonRoll = GC.getGameINLINE().getSorenRandNum(5, "Should planet have a moon?");
		if (iMoonRoll == 0)
		{
			bMoon = true;
		}

		int iRingsRoll = GC.getGameINLINE().getSorenRandNum(5, "Should planet have rings?");
		if (iRingsRoll == 0)
		{
			bRings = true;
		}

		//# Planet type dependant on size
		int iPlanetType = getRandomPlanetType(iPreferredYield, iSize);

		//# All planet values determined, add it to the system

		//#printd("Adding Planet Info: %s (%d), Size %d, Orbit %d, Moon %s, Rings %s" % (aszPlanetTypeNames[iPlanetType], iPlanetType, iSize, iOrbitRing, bMoon, bRings))

		aaiPlanetInfos[iPlanetLoop] = new int[5];
		aaiPlanetInfos[iPlanetLoop][0] = iPlanetType;
		aaiPlanetInfos[iPlanetLoop][1] = (int)iSize;
		aaiPlanetInfos[iPlanetLoop][2] = iOrbitRing;
		aaiPlanetInfos[iPlanetLoop][3] = (int)bMoon;
		aaiPlanetInfos[iPlanetLoop][4] = (int)bRings;
	}
	return aaiPlanetInfos;
}
/*
######################################################
#		Returns an array of randomized orbit ring locations based on the number of planets this system has
######################################################
*/
int* CvMap::getRandomizedOrbitRingArray(int iNumPlanets)
{
	int* aiOrbitRingsArray = new int[8];
	int** aiCalcArray = new int* [8];

	//# Determine randomized list
	for (int iLoop = 0; iLoop < 8 ;iLoop++)
	{
		int iRand = GC.getGameINLINE().getSorenRandNum(100, "Randomized Planet orbit ring roll");
		aiCalcArray[iLoop] = new int[2];
		aiCalcArray[iLoop][0] = iRand;
		aiCalcArray[iLoop][1] = iLoop;
	}
	//# Sort List
	bool bSorted = false;
	while (bSorted != true)
	{
		bSorted = true;
		for (int i = 0; i < 7;i++)
		{
			if (aiCalcArray[i][0] > aiCalcArray[i + 1][0])
			{
				bSorted = false;
				int* temp = new int[2];
				temp[0] = aiCalcArray[i][0];
				temp[1] = aiCalcArray[i][1];
				aiCalcArray[i][0] = aiCalcArray[i + 1][0];
				aiCalcArray[i][1] = aiCalcArray[i + 1][1];
				aiCalcArray[i + 1][0] = temp[0];
				aiCalcArray[i + 1][1] = temp[1];
			}

		}
	}

	//# Determine actual list to return
	for (int iPlanetLoop = 0; iPlanetLoop < iNumPlanets;iPlanetLoop++)
	{
		aiOrbitRingsArray[iPlanetLoop] = aiCalcArray[iPlanetLoop][1];		//# Index 1 is the orbit ring loc, Index 0 is the rand # which we don't care about
	}
	return aiOrbitRingsArray;
}
/*
######################################################
#		Determines the randomized planet type based on the preferred yield
######################################################
*/
PlanetTypes CvMap::getRandomPlanetType(YieldTypes iPreferredYield, PlanetSizes iSize)
{

	//# Pick favored Yield for planet to be

	int iFood = 0;
	int iProduction = 1;
	int iCommerce = 2;

	int iBonusPlanetYieldChance = 80;
	int iDefaultPlanetYieldChance = 33;
	int iNegativePlanetYieldChance = 10;

	PlanetTypes iPlanetType = NO_PLANETTYPE;

	//##### Determine Chosen Favored Yield #####

	int iTotal = 0;
	int Yields = NUM_YIELD_TYPES;
	int* iYieldsChance = new int[Yields];

	for (int i = 0; i < Yields;i++)
	{
		iYieldsChance[i] = 0;
	}

	//# Loop through all yields, see if one is favored
	for (int iYieldLoop = 0;iYieldLoop < Yields; iYieldLoop++)
	{
		if (iPreferredYield != -1)
		{
			//# Is the preferred yield
			if (iYieldLoop == iPreferredYield)
			{
				iYieldsChance[iYieldLoop] = iTotal + iBonusPlanetYieldChance;
				iTotal += iBonusPlanetYieldChance;

			}
			//# Not preferred yield
			else
			{
				iYieldsChance[iYieldLoop] = iTotal + iNegativePlanetYieldChance;
				iTotal += iNegativePlanetYieldChance;
			}
		}
		//# No preferred yields period
		else
		{
			iYieldsChance[iYieldLoop] = iTotal + iDefaultPlanetYieldChance;
			iTotal += iDefaultPlanetYieldChance;
		}
	}
	int iChosenYield = 0;
	int iChooseYieldRand = GC.getGameINLINE().getSorenRandNum(iTotal, "Choosing preferred yield for planet's identity");
	for (int iYieldLoop = 0;iYieldLoop < Yields;iYieldLoop++)
	{
		if (iChooseYieldRand < iYieldsChance[iYieldLoop])
		{
			iChosenYield = iYieldLoop;
			break;
		}
	}
	//##### Chosen yield is picked, now determine this planet's identity from it #####

	//#printd("Yield Chances")
	//#printd(iYieldsChance)
	//#printd("Yield Rand Roll %d" %(iChooseYieldRand))
	//#printd("Chosen Yield Type is %d" %(iChosenYield))

	//# Determine total to roll from
	int iYieldTotal = 0;
	for (int iPlanetTypeLoop = 0;iPlanetTypeLoop < GC.getNumPlanetInfos();iPlanetTypeLoop++)
	{
		
		//# Cannot have a large green planet
		if (GC.getPlanetInfo((PlanetTypes)iPlanetTypeLoop).getMaxSize() < iSize || GC.getPlanetInfo((PlanetTypes)iPlanetTypeLoop).getMinSize() > iSize)
		{
			continue;
		}
		iYieldTotal += m_aaiPlanetYields[iPlanetTypeLoop][iChosenYield];
	}
	int iYieldRand = GC.getGameINLINE().getSorenRandNum(iYieldTotal, "Determining type of planet from preferred yield");

	//#printd("iYieldTotal")
	//#printd(iYieldTotal)
	//#printd("iYieldRand")
	//#printd(iYieldRand)
	//#printd("\n")

	int iCurrentSum = 0;
	for (int iPlanetTypeLoop = 0;iPlanetTypeLoop < GC.getNumPlanetInfos();iPlanetTypeLoop++)
	{

		//# Cannot have a large green planet
		if (GC.getPlanetInfo((PlanetTypes)iPlanetTypeLoop).getMaxSize() < iSize || GC.getPlanetInfo((PlanetTypes)iPlanetTypeLoop).getMinSize() > iSize)
		{
			continue;
		}

		int iTemp = iCurrentSum;
		iCurrentSum += m_aaiPlanetYields[iPlanetTypeLoop][iChosenYield];
		//#printd("%d : iPlanetTypeLoop" % (iPlanetTypeLoop))
		//#printd("   iTemp: %d" % (iTemp))
		//#printd("   iCurrentSum: %d" % (iCurrentSum))
		//#printd("      Hoping iYieldRand (%d) to be less than iCurrentSum (%d)" % (iYieldRand, iCurrentSum))
		if (iYieldRand < iCurrentSum)
		{
			iPlanetType = (PlanetTypes)iPlanetTypeLoop;
			//#printd("Picked %s from range of %d to %d; total %d" % (aszPlanetTypeNames[iPlanetType], iTemp, iCurrentSum, iYieldTotal))
			break;
		}
	}
	FAssert(iPlanetType!=-1,"Trying to add an invalid Planet Type");

	return iPlanetType;

}

int CvMap::getNumSystems() const//this isn't last element - list may have holes
{
	return i_numSolarSystemsSlots;
}



/*
option 1
######################################################
#		Creates a randomized system based on location, preferred yield and preferred number of planets
######################################################

def createRandomSystem(iX, iY, iPreferredYield, iPreferredPlanetQuantity):

	#printd("\n\nCreating system at %d, %d" %(iX, iY))

	pPlot = CyMap().plot(iX, iY)

	bHomeworld = false
	if (pPlot.isCity() or isPlotAPlayersStart(pPlot)):
		bHomeworld = true

	# Random roll to pick sun color... may replace this with something more useful later
	iSunTypeRand = CyGame().getSorenRandNum(iNumSunTypes, "Random roll to determine sun type")

	pSystem = CvSystem(iX,iY,iSunTypeRand)

	# Roll to determine number of planets... quantity can be one of 3 sets based on preferred argument

	iNumPlanetsRand = CyGame().getSorenRandNum(5, "Rolling for number of planets in a system")
	iNumPlanets = iPreferredPlanetQuantity + iNumPlanetsRand + 1

	if (bHomeworld):
		iNumPlanets = 7

	# Loop until needs are met (enough food for starting position)

	aaiPlanetInfos = -1

	bExit = false
	iPass = 0

	# Enter loop if this is a player's start location or if we haven't made a set of planets yet
	while (not bExit):

		aaiPlanetInfos = getRandomizedPlanetInfo(iNumPlanets, iPreferredYield)

		#printd("Candidate Planet Array:")
		#printd(aaiPlanetInfos)

		aiTotalYield = [0,0,0]
		aiMaxPlanetYield = [0,0,0]

		# Only the first 3 planets can be used by default
		aiTotalYieldLevel1 = [0,0,0]
		aiMaxPlanetYieldLevel1 = [0,0,0]

		aiTotalYieldLevel2 = [0,0,0]
		aiMaxPlanetYieldLevel2 = [0,0,0]

		aiTotalYieldLevel3 = [0,0,0]
		aiMaxPlanetYieldLevel3 = [0,0,0]

		iPopulationLimitLevel1 = 0
		iPopulationLimitLevel2 = 0
		iPopulationLimitLevel3 = 0
		iToxicPlanets = 0
		iNeptuneGiants = 0
		iSaturnGiants = 0
		iJovianGiants = 0
		iUranianGiants = 0

		for iPlanetLoop in range(iNumPlanets):

			iPlanetType = aaiPlanetInfos[iPlanetLoop][0]
			iPlanetSize = aaiPlanetInfos[iPlanetLoop][1]
			iOrbitRing = aaiPlanetInfos[iPlanetLoop][2]

			# Count Gas Giants and toxics
			if (aaiPlanetYields[iPlanetType][0] < -1): iToxicPlanets += 1

			for iYieldLoop in range(3):

				# Has yield, add to the total
				if (aaiPlanetYields[iPlanetType][iYieldLoop] > 0):
					aiTotalYield[iYieldLoop] += aaiPlanetYields[iPlanetType][iYieldLoop]

					if (aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYield[iYieldLoop]):
						aiMaxPlanetYield[iYieldLoop] = aaiPlanetYields[iPlanetType][iYieldLoop]


			# Level 1 Planets
			if (iOrbitRing < g_iPlanetRange2):

				# Add to pop
				iPopulationLimitLevel1 += aiDefaultPopulationPlanetSizeTypes[iPlanetSize]

				for iYieldLoop in range(3):

					# Has Yield
					if (aaiPlanetYields[iPlanetType][iYieldLoop] > 0):
						aiTotalYieldLevel1[iYieldLoop] += aaiPlanetYields[iPlanetType][iYieldLoop]

						if (aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYieldLevel1[iYieldLoop]):
							aiMaxPlanetYieldLevel1[iYieldLoop] = aaiPlanetYields[iPlanetType][iYieldLoop]

			# Level 2 Planets
			if (iOrbitRing < g_iPlanetRange3):

				# Add to pop
				iPopulationLimitLevel2 += aiDefaultPopulationPlanetSizeTypes[iPlanetSize]

				for iYieldLoop in range(3):

					# Has Yield
					if (aaiPlanetYields[iPlanetType][iYieldLoop] > 0):
						aiTotalYieldLevel2[iYieldLoop] += aaiPlanetYields[iPlanetType][iYieldLoop]

						if (aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYieldLevel2[iYieldLoop]):
							aiMaxPlanetYieldLevel2[iYieldLoop] = aaiPlanetYields[iPlanetType][iYieldLoop]

			# Level 3 Planets
			if (iOrbitRing >= g_iPlanetRange3):

				# Add to pop
				iPopulationLimitLevel3 += aiDefaultPopulationPlanetSizeTypes[iPlanetSize]

				for iYieldLoop in range(3):

					# Has Yield
					if (aaiPlanetYields[iPlanetType][iYieldLoop] > 0):
						aiTotalYieldLevel3[iYieldLoop] += aaiPlanetYields[iPlanetType][iYieldLoop]

						if (aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYieldLevel3[iYieldLoop]):
							aiMaxPlanetYieldLevel3[iYieldLoop] = aaiPlanetYields[iPlanetType][iYieldLoop]

		# If it's someone's starting system then it needs to meet a few criteria

		if (bHomeworld):
			# Need at least 5 food total
			if (aiTotalYield[0] >= 5):
				# Needs a 4 food planet (home) inside Level 1 or a 3 food(mars) with a 5+ food (gia or swamp) level 2
				if (aiMaxPlanetYieldLevel1[0] >= 4) or (aiMaxPlanetYieldLevel1[0] == 3 and aiMaxPlanetYieldLevel2[0] >= 5):
					# Needs a 2 prod planet inside Level 1
					if (aiMaxPlanetYieldLevel1[1] >= 2):
						# Needs at least 5 pop support inside Level 1
						if (iPopulationLimitLevel1 >= 5):
							bExit = true

		# If not a starting system, we're less picky
		else:
			bExit = true

		# No more than 1 toxic planets
		if (iToxicPlanets > 1):
			bExit = false

		# Need a certain level of Food in Level 1 no matter what the system is
		if (aiMaxPlanetYieldLevel1[0] < 2):
			bExit = false

		if (aiTotalYield[0] < 2):
			bExit = false

		# Need a certain level of Food in Level 2 no matter what the system is
		if (aiMaxPlanetYieldLevel2[0] < 2):
			bExit = false

		# Not too much
		if (aiTotalYield[0] >= 13):
			bExit = false

		# Need a certain level of production in Level 1
		if (aiTotalYieldLevel1[1] < 2):
			bExit = false

		# Needs at least 4 pop support inside Level 1
		if (iPopulationLimitLevel1 < 4):
			bExit = false

		# Needs a gas giant or ice or toxic in Level 2 or 3
		if (aiMaxPlanetYieldLevel2[2] < 4) or (aiMaxPlanetYieldLevel3[2] < 4):
			bExit = false

		if (not bExit):
			#printd("Found a bad System setup, making another\n")
			iPass += 1

	printd("\n%d Passes to create valid system" %(iPass))

	if (bHomeworld):
		printd("*** Is Player Starting System")

	printd("Array to generate planets from:")
	printd(aaiPlanetInfos)

	# Now that the planets have been verified, actually create the system
	for iPlanetLoop in range(iNumPlanets):

		printd("Adding Planet %d of %d to system" %(iPlanetLoop, iNumPlanets))

		pSystem.addPlanet(aaiPlanetInfos[iPlanetLoop][0], aaiPlanetInfos[iPlanetLoop][1], aaiPlanetInfos[iPlanetLoop][2], aaiPlanetInfos[iPlanetLoop][3], aaiPlanetInfos[iPlanetLoop][4])

		# Name our newly created planet
		szName = ""
		iPlanetColumns = [0, 0]

		while (iPlanetColumns[0] == iPlanetColumns[1]):
			iPlanetColumns[0] = CyGame().getSorenRandNum(3, "Getting random planet name :)")
			iPlanetColumns[1] = CyGame().getSorenRandNum(3, "Getting random planet name 2 :)")

		for iColumnLoop in range(2):
			# Column 0
			if (iPlanetColumns[iColumnLoop] == 0):
				iWordRand = CyGame().getSorenRandNum(len(g_aszRandomPlanetNames0), "Random word for planet name")
				szName += localText.getText(g_aszRandomPlanetNames0[iWordRand], ()) + " "
			# Column 1
			elif (iPlanetColumns[iColumnLoop] == 1):
				iWordRand = CyGame().getSorenRandNum(len(g_aszRandomPlanetNames1), "Random word for planet name")
				szName += localText.getText(g_aszRandomPlanetNames1[iWordRand], ()) + " "
			# Column 2
			elif (iPlanetColumns[iColumnLoop] == 2):
				iWordRand = CyGame().getSorenRandNum(len(g_aszRandomPlanetNames2), "Random word for planet name")
				szName += localText.getText(g_aszRandomPlanetNames2[iWordRand], ()) + " "
		szName += " : Class "

		iPlanetType = aaiPlanetInfos[iPlanetLoop][0]
		iPlanetSize = aaiPlanetInfos[iPlanetLoop][1]

		if (iPlanetSize == iPlanetSizeHuge):
			if (iPlanetType == iPlanetTypeJovian1): szName += "Td"
			if (iPlanetType == iPlanetTypeJovian2): szName += "Td"
			if (iPlanetType == iPlanetTypeJovian3): szName += "Tl"
			if (iPlanetType == iPlanetTypeJovian4): szName += "Tg"
			if (iPlanetType == iPlanetTypeYellow): szName += "Tn"
			if (iPlanetType == iPlanetTypeSaturn1): szName += "Td"
			if (iPlanetType == iPlanetTypeSaturn2): szName += "Tl"
			if (iPlanetType == iPlanetTypeSaturn3): szName += "Tp"
			if (iPlanetType == iPlanetTypeSaturn4): szName += "Th"
			if (iPlanetType == iPlanetTypeUranian1): szName += "Tn"
			if (iPlanetType == iPlanetTypeUranian2): szName += "Tl"
			if (iPlanetType == iPlanetTypeUranian3): szName += "Tp"
			if (iPlanetType == iPlanetTypeUranian4): szName += "Tp"
			if (iPlanetType == iPlanetTypeBlue): szName += "Tg"
			if (iPlanetType == iPlanetTypeNeptune1): szName += "Td"
			if (iPlanetType == iPlanetTypeNeptune2): szName += "Tl"
			if (iPlanetType == iPlanetTypeNeptune3): szName += "Tp"
			if (iPlanetType == iPlanetTypeNeptune4): szName += "Tg"

		if (iPlanetSize == iPlanetSizeVeryLarge):
			if (iPlanetType == iPlanetTypeJovian1): szName += "Sd"
			if (iPlanetType == iPlanetTypeJovian2): szName += "Sd"
			if (iPlanetType == iPlanetTypeJovian3): szName += "Sl"
			if (iPlanetType == iPlanetTypeJovian4): szName += "Sg"
			if (iPlanetType == iPlanetTypeYellow): szName += "Sn"
			if (iPlanetType == iPlanetTypeSaturn1): szName += "Sd"
			if (iPlanetType == iPlanetTypeSaturn2): szName += "Sl"
			if (iPlanetType == iPlanetTypeSaturn3): szName += "Sp"
			if (iPlanetType == iPlanetTypeSaturn4): szName += "Sh"
			if (iPlanetType == iPlanetTypeUranian1): szName += "Sn"
			if (iPlanetType == iPlanetTypeUranian2): szName += "Sl"
			if (iPlanetType == iPlanetTypeUranian3): szName += "Sp"
			if (iPlanetType == iPlanetTypeUranian4): szName += "Sp"
			if (iPlanetType == iPlanetTypeBlue): szName += "Sd"
			if (iPlanetType == iPlanetTypeNeptune1): szName += "Sd"
			if (iPlanetType == iPlanetTypeNeptune2): szName += "Sl"
			if (iPlanetType == iPlanetTypeNeptune3): szName += "Sp"
			if (iPlanetType == iPlanetTypeNeptune4): szName += "Sg"

		if (iPlanetSize == iPlanetSizeLarge):
			if (iPlanetType == iPlanetTypeJovian1): szName += "Jd"
			if (iPlanetType == iPlanetTypeJovian2): szName += "Jd"
			if (iPlanetType == iPlanetTypeJovian3): szName += "Jl"
			if (iPlanetType == iPlanetTypeJovian4): szName += "Jg"
			if (iPlanetType == iPlanetTypeYellow): szName += "Jn"
			if (iPlanetType == iPlanetTypeSaturn1): szName += "Jd"
			if (iPlanetType == iPlanetTypeSaturn2): szName += "Jl"
			if (iPlanetType == iPlanetTypeSaturn3): szName += "Jp"
			if (iPlanetType == iPlanetTypeSaturn4): szName += "Jh"
			if (iPlanetType == iPlanetTypeUranian1): szName += "Jn"
			if (iPlanetType == iPlanetTypeUranian2): szName += "Jl"
			if (iPlanetType == iPlanetTypeUranian3): szName += "Jp"
			if (iPlanetType == iPlanetTypeUranian4): szName += "Jp"
			if (iPlanetType == iPlanetTypeBlue): szName += "Jd"
			if (iPlanetType == iPlanetTypeNeptune1): szName += "Jd"
			if (iPlanetType == iPlanetTypeNeptune2): szName += "Jl"
			if (iPlanetType == iPlanetTypeNeptune3): szName += "Jp"
			if (iPlanetType == iPlanetTypeNeptune4): szName += "Jg"

		if (iPlanetType == iPlanetTypeBrown1): szName += "L"
		if (iPlanetType == iPlanetTypeBrown2): szName += "L"
		if (iPlanetType == iPlanetTypeBrown3): szName += "L"
		if (iPlanetType == iPlanetTypeBrown4): szName += "L"

		if (iPlanetType == iPlanetTypeRed): szName += "G"
		if (iPlanetType == iPlanetTypeLava1): szName += "G"
		if (iPlanetType == iPlanetTypeLava2): szName += "G"
		if (iPlanetType == iPlanetTypeLava3): szName += "G"
		if (iPlanetType == iPlanetTypeLava4): szName += "G"

		if (iPlanetType == iPlanetTypeGreen): szName += "M"
		if (iPlanetType == iPlanetTypeHome1): szName += "M"
		if (iPlanetType == iPlanetTypeHome2): szName += "M"
		if (iPlanetType == iPlanetTypeHome3): szName += "M"
		if (iPlanetType == iPlanetTypeHome4): szName += "M"

		if (iPlanetType == iPlanetTypeOrange): szName += "K"
		if (iPlanetType == iPlanetTypeMars1): szName += "H"
		if (iPlanetType == iPlanetTypeMars2): szName += "H"
		if (iPlanetType == iPlanetTypeMars3): szName += "K"
		if (iPlanetType == iPlanetTypeMars4): szName += "K"

		if (iPlanetType == iPlanetTypeGia1): szName += "Mg"
		if (iPlanetType == iPlanetTypeGia2): szName += "Mg"
		if (iPlanetType == iPlanetTypeGia3): szName += "Mg"
		if (iPlanetType == iPlanetTypeGia4): szName += "Mg"

		if (iPlanetType == iPlanetTypeSwamp1): szName += "Ms"
		if (iPlanetType == iPlanetTypeSwamp2): szName += "Ms"
		if (iPlanetType == iPlanetTypeSwamp3): szName += "Ms"
		if (iPlanetType == iPlanetTypeSwamp4): szName += "Ms"

		if (iPlanetType == iPlanetTypeToxic1): szName += "N"
		if (iPlanetType == iPlanetTypeToxic2): szName += "Y"
		if (iPlanetType == iPlanetTypeToxic3): szName += "Y"
		if (iPlanetType == iPlanetTypeToxic4): szName += "Y"

		if (iPlanetType == iPlanetTypeWhite): szName += "P"
		if (iPlanetType == iPlanetTypeIce1): szName += "P"
		if (iPlanetType == iPlanetTypeIce2): szName += "P"
		if (iPlanetType == iPlanetTypeIce3): szName += "P"
		if (iPlanetType == iPlanetTypeIce4): szName += "P"
		if (iPlanetType == iPlanetTypeIce5): szName += "P"
		if (iPlanetType == iPlanetTypeIce6): szName += "P"
		if (iPlanetType == iPlanetTypeIce7): szName += "P"
		if (iPlanetType == iPlanetTypeIce8): szName += "P"


		if (iPlanetType == iPlanetTypeGray): szName += "D"
		if (iPlanetType == iPlanetTypeGray1): szName += "D"
		if (iPlanetType == iPlanetTypeGray2): szName += "D"
		if (iPlanetType == iPlanetTypeGray3): szName += "D"
		if (iPlanetType == iPlanetTypeGray4): szName += "D"
		if (iPlanetType == iPlanetTypeGray5): szName += "D"
		if (iPlanetType == iPlanetTypeGray6): szName += "D"
		if (iPlanetType == iPlanetTypeGray7): szName += "D"
		if (iPlanetType == iPlanetTypeGray8): szName += "D"
		if (iPlanetType == iPlanetTypeGray9): szName += "D"
		if (iPlanetType == iPlanetTypeGray10): szName += "D"
		if (iPlanetType == iPlanetTypeGray11): szName += "D"
		if (iPlanetType == iPlanetTypeGray12): szName += "D"
		if (iPlanetType == iPlanetTypeGray13): szName += "D"
		if (iPlanetType == iPlanetTypeGray14): szName += "D"
		if (iPlanetType == iPlanetTypeGray15): szName += "D"
		if (iPlanetType == iPlanetTypeGray16): szName += "D"

		iGasGiant = 0
		if (iPlanetType == iPlanetTypeJovian1 or iPlanetType == iPlanetTypeJovian2 or iPlanetType == iPlanetTypeJovian3 or iPlanetType == iPlanetTypeJovian4  or iPlanetType == iPlanetTypeYellow or iPlanetType == iPlanetTypeSaturn1 or iPlanetType == iPlanetTypeSaturn2 or iPlanetType == iPlanetTypeSaturn3 or iPlanetType == iPlanetTypeSaturn4 or iPlanetType == iPlanetTypeUranian1 or iPlanetType == iPlanetTypeUranian2 or iPlanetType == iPlanetTypeUranian3 or iPlanetType == iPlanetTypeUranian4 or iPlanetType == iPlanetTypeBlue or iPlanetType == iPlanetTypeNeptune1 or iPlanetType == iPlanetTypeNeptune2 or iPlanetType == iPlanetTypeNeptune3 or iPlanetType == iPlanetTypeNeptune4):
			iGasGiant = 1
		else:
			if (iPlanetSize == iPlanetSizeLarge):
				szName += "!High G!"

		pSystem.getPlanetByIndex(iPlanetLoop).setName(szName)

	# Find best planet if homeworld, add some stuff
	if (bHomeworld):

		if (pPlot.isCity()):

			# Assign city name to best planet
			iBestPlanet = getBestPlanetInSystem(pSystem)
			pBestPlanet = pSystem.getPlanetByIndex(iBestPlanet)
			# pBestPlanet.setName(pPlot.getPlotCity().getName())
			pBestPlanet.setHasBuilding(CvUtil.findInfoTypeNum(gc.getBuildingInfo,gc.getNumBuildingInfos(),'BUILDING_CAPITOL'), true)

			addBasicBuildingsToBestPlanet(pSystem)

	# Set the default selected & building planet to the best one
	pBestPlanet = pSystem.getPlanetByIndex(getBestPlanetInSystem(pSystem))
	pSystem.setSelectedPlanet(pBestPlanet.getOrbitRing())
	pSystem.setBuildingPlanetRing(pBestPlanet.getOrbitRing())

	pSystem.updateDisplay()

	return pSystem
	*/
	//option 2 (preferred)
CvSolarSystem* CvMap::createRandomSystem(int iX, int iY, YieldTypes iPreferredYield, int iPreferredPlanetQuantity)
{

	//#printd("\n\nCreating system at %d, %d" %(iX, iY))

	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(iX, iY);


	bool bHomeworld = false;
	//FAssertMsg(isPlotAPlayersStart(pPlot),"report 1 ss");
	//FAssertMsg(!isPlotAPlayersStart(pPlot),"report 1 h");
	if (pPlot->isCity() || isPlotAPlayersStart(pPlot))
	{
		bHomeworld = true;
	}
	//# Random roll to pick sun color... may replace this with something more useful later
	int iSunTypeRand = GC.getGameINLINE().getSorenRandNum(GC.getNumStarInfos(), "Random roll to determine sun type");
	//FAssertMsg(false,"system generating1");
	CvSolarSystem* pSystem = new CvSolarSystem();
	
	pSystem->reset(iX, iY, false, (StarTypes)iSunTypeRand);
	//# Roll to determine number of planets... quantity can be one of 3 sets based on preferred argument

	int iNumPlanetsRand = GC.getGameINLINE().getSorenRandNum(6, "Rolling for number of planets in a system");
	int iNumPlanets = iPreferredPlanetQuantity + iNumPlanetsRand;

	if (bHomeworld)
	{
		iNumPlanets = 6;
	}

	//# Loop until needs are met(enough food for starting position)

	int** aaiPlanetInfos = NULL;

	bool bExit = false;
	int iPass = 0;
	//# Enter loop if this is a player's start location or if we haven't made a set of planets yet
	while (!bExit)
	{
		//FAssertMsg(false,"system generating3");
		aaiPlanetInfos = getRandomizedPlanetInfo(iNumPlanets, iPreferredYield);

		//#printd("Candidate Planet Array:")
		//#printd(aaiPlanetInfos)


		int* aiTotalYield = new int[NUM_YIELD_TYPES];
		int* aiMaxPlanetYield = new int[NUM_YIELD_TYPES];

		//# Only the first 3 planets can be used by default
		int* aiTotalYieldLevel1 = new int[NUM_YIELD_TYPES];
		int* aiMaxPlanetYieldLevel1 = new int[NUM_YIELD_TYPES];

		int* aiTotalYieldLevel2 = new int[NUM_YIELD_TYPES];
		int* aiMaxPlanetYieldLevel2 = new int[NUM_YIELD_TYPES];

		for (int il = 0; il < NUM_YIELD_TYPES;il++)
		{
			aiTotalYield[il] = 0;
			aiMaxPlanetYield[il] = 0;

			aiTotalYieldLevel1[il] = 0;
			aiMaxPlanetYieldLevel1[il] = 0;

			aiTotalYieldLevel2[il] = 0;
			aiMaxPlanetYieldLevel2[il] = 0;
		}

		int iPopulationLimitLevel1 = 0;
		int iPopulationLimitLevel2 = 0;

		for (int iPlanetLoop = 0;iPlanetLoop < iNumPlanets; iPlanetLoop++)
		{

			int iPlanetType = aaiPlanetInfos[iPlanetLoop][0];
			int iPlanetSize = aaiPlanetInfos[iPlanetLoop][1];
			int iOrbitRing = aaiPlanetInfos[iPlanetLoop][2];

			for (int iYieldLoop = 0;iYieldLoop < NUM_YIELD_TYPES;iYieldLoop++)
			{

				//# Has yield, add to the total Aradziem-may be below zero
				if (m_aaiPlanetYields[iPlanetType][iYieldLoop] != 0)
				{
					aiTotalYield[iYieldLoop] += m_aaiPlanetYields[iPlanetType][iYieldLoop];

					if (m_aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYield[iYieldLoop])
					{
						aiMaxPlanetYield[iYieldLoop] = m_aaiPlanetYields[iPlanetType][iYieldLoop];
					}
				}
			}
			//# Level 1 Planets
			if (iOrbitRing < m_g_iPlanetRange2)
			{
				//# Add to pop
				iPopulationLimitLevel1 += iPlanetSize + 1;

				for (int iYieldLoop = 0;iYieldLoop < NUM_YIELD_TYPES;iYieldLoop++)
				{

					//# Has Yield
					if (m_aaiPlanetYields[iPlanetType][iYieldLoop] > 0)
					{
						aiTotalYieldLevel1[iYieldLoop] += m_aaiPlanetYields[iPlanetType][iYieldLoop];

						if (m_aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYieldLevel1[iYieldLoop])
						{
							aiMaxPlanetYieldLevel1[iYieldLoop] = m_aaiPlanetYields[iPlanetType][iYieldLoop];
						}
					}
				}
			}
			//# Level 2 Planets
			if (iOrbitRing < m_g_iPlanetRange3)
			{

				//# Add to pop
				iPopulationLimitLevel2 += iPlanetSize + 1;

				for (int iYieldLoop = 0;iYieldLoop < NUM_YIELD_TYPES;iYieldLoop++)
				{

					//# Has Yield
					if (m_aaiPlanetYields[iPlanetType][iYieldLoop] > 0)
					{
						aiTotalYieldLevel2[iYieldLoop] += m_aaiPlanetYields[iPlanetType][iYieldLoop];

						if (m_aaiPlanetYields[iPlanetType][iYieldLoop] > aiMaxPlanetYieldLevel2[iYieldLoop])
						{
							aiMaxPlanetYieldLevel2[iYieldLoop] = m_aaiPlanetYields[iPlanetType][iYieldLoop];
						}
					}
				}
			}
		}
		//# If it's someone's starting system then it needs to meet a few criteria
		if (bHomeworld)
		{
			//# Need at least 6 food total
			if (aiTotalYield[0] >= 6)
			{
				//# Needs a 5 food planet(Green) inside Level 1
				if (aiMaxPlanetYieldLevel1[0] >= 3)
				{
					//# Needs at least 4 pop support inside Level 1
					if (iPopulationLimitLevel1 >= 4)
					{
						bExit = true;
					}
				}
			}
		}
		//# If not a starting system, we're less picky
		else
		{
			bExit = true;
		}

		//# Need a certain level of Food in Level 1 no matter what the system is
		if (aiMaxPlanetYieldLevel1[0] < 2)
		{
			bExit = false;
		}

		//# Need a certain level of production in Level 1
		if (aiTotalYieldLevel1[1] < 3)
		{
			bExit = false;
		}

		//# Needs at least 2 pop support inside Level 1
		if (iPopulationLimitLevel1 < 2)
		{
			bExit = false;
		}

		if (!bExit)
		{
			//#printd("Found a bad System setup, making another\n")
			iPass += 1;
		}
	}

	//printd("\n%d Passes to create valid system" % (iPass))

	if (bHomeworld)
	{
		//printd("*** Is Player Starting System")
	}

	//printd("Array to generate planets from:")
	//printd(aaiPlanetInfos)

	//# Now that the planets have been verified, actually create the system
	//# CP - Planet Name : all planets in system get same "last name" (start section)
	int* iPlanetColumns = new int[2];
	iPlanetColumns[0]=0;
	iPlanetColumns[1]=0;
	CvWString szName1 = "";
	iPlanetColumns[1] = GC.getGameINLINE().getSorenRandNum(3, "Getting random planet name :)");
	int iWordRand;
	//# Column 0
	if (iPlanetColumns[1] == 0)
	{
		iWordRand = GC.getGameINLINE().getSorenRandNum(m_i_RandomPlanetNames0, "Random word for planet name");
		szName1 = gDLL->getText(m_g_aszRandomPlanetNames0[iWordRand]) + " ";
	}
	//# Column 1
	else if (iPlanetColumns[1] == 1)
	{
		iWordRand = GC.getGameINLINE().getSorenRandNum(m_i_RandomPlanetNames0, "Random word for planet name");
		szName1 = gDLL->getText(m_g_aszRandomPlanetNames1[iWordRand]) + " ";
	}
	//# Column 2
	else if (iPlanetColumns[1] == 2)
	{
		iWordRand = GC.getGameINLINE().getSorenRandNum(m_i_RandomPlanetNames0, "Random word for planet name");
		szName1 = gDLL->getText(m_g_aszRandomPlanetNames2[iWordRand]) + " ";
	}
	//# CP - Planet Name : all planets in system get same "last name" (end section, more below)
	//# CP - Planet Name : different planets in the same system don't get the same name
	//# CP - end section

	for (int iPlanetLoop = 0;iPlanetLoop < iNumPlanets; iPlanetLoop++)
	{
		//printd("Adding Planet %d of %d to system" % (iPlanetLoop, iNumPlanets))

		pSystem->addPlanet(aaiPlanetInfos[iPlanetLoop][0], aaiPlanetInfos[iPlanetLoop][1], aaiPlanetInfos[iPlanetLoop][2],(bool) aaiPlanetInfos[iPlanetLoop][3],(bool) aaiPlanetInfos[iPlanetLoop][4]);
	}
	for (int iPlanetLoop = 0;iPlanetLoop < iNumPlanets; iPlanetLoop++)
	{
		//# Name our newly created planet
		CvWString szName = "";
		//#iPlanetColumns = [0, 0] # CP - Planet Name : only the "first name" changes for planets in a system, so :
		iPlanetColumns[0] = iPlanetColumns[1];

		while (iPlanetColumns[0] == iPlanetColumns[1])
		{
			iPlanetColumns[0] = 1 + GC.getGameINLINE().getSorenRandNum(3, "Getting random planet name :)");// # CP - Planet Name
		}
		CvWString* localPlanetNames;
		int iLocalPlanetNames;
		//# Column 1
		if (iPlanetColumns[0] == 1)
		{
			localPlanetNames = m_g_aszRandomPlanetNames1;
			iLocalPlanetNames = m_i_RandomPlanetNames1;
		}
		//# Column 2
		else if (iPlanetColumns[0] == 2)
		{
			localPlanetNames = m_g_aszRandomPlanetNames2;
			iLocalPlanetNames = m_i_RandomPlanetNames2;
		}
		//# Column 3
		else if (iPlanetColumns[0] == 3)
		{
			localPlanetNames = m_g_aszRandomPlanetNames3;
			iLocalPlanetNames = m_i_RandomPlanetNames3;
		}

		iWordRand = GC.getGameINLINE().getSorenRandNum(iLocalPlanetNames, "Random word for planet name");
		szName += gDLL->getText(localPlanetNames[iWordRand]) + " ";

		//probably not a good idea in C++
		//delete localPlanetNames[iWordRand];// # remove the name we just used from the list we just used

		szName += szName1;// # CP - Planet Name

		pSystem->getPlanetByIndex(iPlanetLoop)->setName(szName);
	}
	//# Find best planet if homeworld, add some stuff

	//# Set the default selected & building planet to the best one
	CvPlanet* pBestPlanet = pSystem->getPlanetByIndex(pSystem->getBestPlanetInSystem());
	pSystem->setSelectedPlanet(pSystem->getPlanetIndexByRing(pBestPlanet->getOrbitRing()));
	pSystem->setBuildingPlanetRing(pSystem->getPlanetIndexByRing(pBestPlanet->getOrbitRing()));

	if (bHomeworld)
	{
		if (pPlot->isCity())
		{
			CvCity* pCity = pPlot->getPlotCity();
			//# Assign city name to best planet
			int iBestPlanet = pSystem->getBestPlanetInSystem();
			CvPlanet* pBestPlanet = pSystem->getPlanetByIndex(iBestPlanet);
			pBestPlanet->setName(pCity->getName());


			int iCapitolClass = GC.getInfoTypeForString(GC.getDefineSTRING("FF_PALACE_BUILDINGCLASS"));
			int iCapitol = GC.getCivilizationInfo(pCity->getCivilizationType()).getCivilizationBuildings(iCapitolClass);
			pBestPlanet->setHasBuilding(iCapitol, true);

			pSystem->addBasicBuildingsToBestPlanet();
			//CvCity* testCity = pPlot->getPlotCity();
		}
	}

	

	pSystem->updateDisplay();
	//FAssertMsg(false,"system generated");
	//CvFeature* testl = pPlot->getFeatureSymbol();
	return pSystem;
}

//########## Planet Based Resources  - CP added 8-Mar-2010 ##########
void CvMap::AddPlanetaryResources()
{
	//'Add a resource to a planet in some of the star systems.'
		
	int iGrain = GC.getInfoTypeForString("BONUS_WHEAT");
	int iCattle = GC.getInfoTypeForString("BONUS_COW");
	int iSeafood = GC.getInfoTypeForString("BONUS_FISH");
	int iSpices = GC.getInfoTypeForString("BONUS_SPICES");
	int iWine = GC.getInfoTypeForString("BONUS_WINE");
	int iCotton = GC.getInfoTypeForString("BONUS_COTTON");
		
	//# determine how many of each of the 6 resources to add to the map, pre random variation
	int iBaseNumEachType = GC.getGameINLINE().countCivPlayersEverAlive() / 2;
	//printd( "Base planet resource count = %d per type" % iBaseNumEachType)
		
	/*# Make a list of resources to assign, with a randomized list of the "base number" of each
	# to which will be appended a randomized list of the "extra". It is done this way because the
	# combined list can actually exceed the number of systems if the "sparse" setting is used
	# on a duel or tiny map, and maybe even a small one. The randomization method insures a
	# random selection of resources.*/
	//Aradziem bonuses first because I know how long list has to be.
	//I mean the additional ones.
	int iWorldSize = getWorldSize();
	int iExtraGrain;
	int iExtraCattle;
	int iExtraSeafood;
	int iExtraSpices;
	int iExtraWine;
	int iExtraCotton;
	if ((iWorldSize == 0) || (iWorldSize == 1)) //duel or tiny = 0 - 1 extra for each
	{
		iExtraGrain = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-1");
		iExtraCattle = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-2");
		iExtraSeafood = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-3");
		iExtraSpices = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-4");
		iExtraWine = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-5");
		iExtraCotton = GC.getGameINLINE().getSorenRandNum(2, "variable planet resource count A-6");
	}
	else //# 0 - 2 extra for each
	{
		iExtraGrain = GC.getGameINLINE().getSorenRandNum(3+ iWorldSize/4, "variable planet resource count B-1");
		iExtraCattle = GC.getGameINLINE().getSorenRandNum(3 + iWorldSize / 4, "variable planet resource count B-2");
		iExtraSeafood = GC.getGameINLINE().getSorenRandNum(3 + iWorldSize / 4, "variable planet resource count B-3");
		iExtraSpices = GC.getGameINLINE().getSorenRandNum(3 + iWorldSize / 4, "variable planet resource count B-4");
		iExtraWine = GC.getGameINLINE().getSorenRandNum(3 + iWorldSize / 4, "variable planet resource count B-5");
		iExtraCotton = GC.getGameINLINE().getSorenRandNum(3 + iWorldSize / 4, "variable planet resource count B-6");
	}
	int sum1 = iExtraGrain + iExtraCattle + iExtraSeafood + iExtraSpices + iExtraWine + iExtraCotton;
	int** ltBonuses;
	//note that not all of these systems can have a resource
	if (getNumSystems() <= sum1 + iBaseNumEachType * 6)
	{
		//a bit less randomness
		ltBonuses = new int*[iBaseNumEachType * 6];
		sum1 = 0;
	}
	else
	{
		//int checksum = 0;
		bool sorted = false;
		ltBonuses = new int*[sum1 + iBaseNumEachType * 6];
		for (int iCount = 0;iCount < sum1;iCount++)
		{
			ltBonuses[iCount] = new int[2];
			if (iCount < iExtraGrain)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Grain");
				ltBonuses[iCount][1] = iGrain;
			}
			else if (iCount < iExtraGrain + iExtraCattle)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Cattle");
				ltBonuses[iCount][1] = iCattle;
			}
			else if (iCount < iExtraGrain + iExtraCattle + iExtraSeafood)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Seafood");
				ltBonuses[iCount][1] = iSeafood;
			}
			else if (iCount < iExtraGrain + iExtraCattle + iExtraSeafood + iSpices)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Spices");
				ltBonuses[iCount][1] = iSpices;
			}
			else if (iCount < iExtraGrain + iExtraCattle + iExtraSeafood + iSpices + iWine)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Wine");
				ltBonuses[iCount][1] = iWine;
			}
			else if (iCount < iExtraGrain + iExtraCattle + iExtraSeafood + iSpices + iWine + iCotton)
			{
				ltBonuses[iCount][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization extra Cotton");
				ltBonuses[iCount][1] = iCotton;
			}
		}
		//# randomize the list order
		while (!sorted)
		{
			sorted = true;
			for (int i = 1; i< sum1 ;i++)
			{
				if (ltBonuses[i - 1][0] < ltBonuses[i][0])
				{
					int* temp = new int[2];
					temp = ltBonuses[i];
					ltBonuses[i] = ltBonuses[i - 1];
					ltBonuses[i - 1] = temp;
					sorted = false;
				}
			}
		}
	}
	for (int iCount = 0;iCount < iBaseNumEachType; iCount++)
	{
		ltBonuses[iCount+sum1] = new int[2];
		ltBonuses[iCount+sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Grain");
		ltBonuses[iCount+sum1][1] = iGrain;

		ltBonuses[iCount+ iBaseNumEachType + sum1] = new int[2];
		ltBonuses[iCount + iBaseNumEachType + sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Cattle");
		ltBonuses[iCount + iBaseNumEachType  + sum1][1] = iCattle;

		ltBonuses[iCount + iBaseNumEachType * 2 + sum1] = new int[2];
		ltBonuses[iCount + iBaseNumEachType * 2 + sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Seafood");
		ltBonuses[iCount + iBaseNumEachType * 2 + sum1][1] = iSeafood;

		ltBonuses[iCount + iBaseNumEachType * 3 + sum1] = new int[2];
		ltBonuses[iCount + iBaseNumEachType * 3 + sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Spices");
		ltBonuses[iCount + iBaseNumEachType * 3 + sum1][1] = iSpices;

		ltBonuses[iCount + iBaseNumEachType * 4 + sum1] = new int[2];
		ltBonuses[iCount + iBaseNumEachType * 4 + sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Wine");
		ltBonuses[iCount + iBaseNumEachType * 4 + sum1][1] = iWine;

		ltBonuses[iCount + iBaseNumEachType * 5 + sum1] = new int[2];
		ltBonuses[iCount + iBaseNumEachType * 5 + sum1][0] = GC.getGameINLINE().getSorenRandNum(9999, "resource list randomization Cotton");
		ltBonuses[iCount + iBaseNumEachType * 5 + sum1][1] = iCotton;
	}
	bool bsorted = false;
	while (!bsorted)
	{
		bsorted=true;
		for(int i = sum1+1;i < sum1 + iBaseNumEachType * 6;i++)
		{
			if (ltBonuses[i - 1][0] < ltBonuses[i][0])
			{
				int* temp = new int[2];
				temp = ltBonuses[i];
				ltBonuses[i] = ltBonuses[i - 1];
				ltBonuses[i - 1] = temp;
				bsorted = false;
			}
		}
	}
	
	//# randomize the list order
	//ltBonuses.sort()


	//printd("Initial number of planet resosurces = %d" % len(ltBonuses))		
		
		
	//# append extras to bonus list
	//Aradziem extras included in base list
	//ltBonuses += ltExtraBonuses
	//printd("  Number of extra bonuses (random) = %d" % len(ltExtraBonuses))
	//printd("Final number of planet resources = %d" % len(ltBonuses))
		
	CvSolarSystem** ltSystems = new CvSolarSystem*[getNumSystems()];
	int* iRandoms = new int[getNumSystems()];
	int isystems =0;
	//# make local copy of the list of systems excluding any that are home systems or have 
	//# no planets that give food (which ought to be impossible) in tuples with random numbers
	for (int iSystemLoop = 0;iSystemLoop < getNumSystems();iSystemLoop++) // #FFPBUG
	{
		CvSolarSystem* pSystem ;
		if (m_solarSystems[iSystemLoop] != NULL)
		{
			pSystem = m_solarSystems[iSystemLoop]; //#FFPBUG
		}
		else
		{
			continue;
		}
		bool bHasFood = false;
		for (int iPlanetLoop = 0; iPlanetLoop < pSystem->getNumPlanets();iPlanetLoop++)
		{
			CvPlanet* pPlanet = pSystem->getPlanetByIndex(iPlanetLoop);
			if (pPlanet->getBaseYield(YIELD_FOOD) > 0)
			{
				bHasFood = true;
				break;
			}
		}
		FAssertMsg(bHasFood, "System has no food!");
		CvPlot* pPlot = plot(pSystem->getX(), pSystem->getY());
		if (!(pPlot->isCity() || isPlotAPlayersStart(pPlot) || (!bHasFood)))
		{
			
			ltSystems[isystems] = pSystem;
			iRandoms[isystems] = GC.getGameINLINE().getSorenRandNum(9999, "planet resource randomization Systems");
			isystems++;
		}
	}
				
	bsorted = false;
	while (!bsorted)
	{
		bsorted = true;
		for (int i =  1;i < isystems;i++)
		{
			if (iRandoms[i - 1] < iRandoms[i])
			{
				int temp;
				CvSolarSystem* tempsys;
				temp = iRandoms[i];
				iRandoms[i] = iRandoms[i - 1];
				iRandoms[i - 1] = temp;
				tempsys = ltSystems[i];
				ltSystems[i] = ltSystems[i - 1];
				ltSystems[i - 1] = tempsys;
				bsorted = false;
			}
		}
	}
	//# randomize the local systems list
	//ltSystems.sort()
		
	//printd("Number of suitable systems = %d" % len(ltSystems))
		
	//# OK, now assign the randomized bonuses to random planets in the randomized systems.
	//# Only assign a number of bonuses that is equal to the smaller of the length of
	//# the bonus list and the length of the system list.
	int iNumToAssign = std::min(sum1 + iBaseNumEachType * 6, isystems);
	//Aradziem note that I loop from last to first
	for (int iAssignLoop = iNumToAssign-1; iAssignLoop >= 0;iAssignLoop--)
	{
		CvPlanet** lPlanets = new CvPlanet*[ltSystems[iAssignLoop]->getNumPlanets()];
		int iNumPlanets = 0;
		for (int iPlanet = 0;iPlanet < ltSystems[iAssignLoop]->getNumPlanets();iPlanet++)
		{
			CvPlanet* pLoopPlanet = ltSystems[iAssignLoop]->getPlanetByIndex(iPlanet);
			if (pLoopPlanet->getBaseYield(YIELD_FOOD) > 0)  //# yield 0 is food
			{
				lPlanets[iNumPlanets] = pLoopPlanet;
				iNumPlanets++;
			}
		}
		int iUsePlanet = GC.getGameINLINE().getSorenRandNum(iNumPlanets, "planet resource randomization Planet");
		CvPlanet* pPlanet = lPlanets[iUsePlanet];
		int iBonus = ltBonuses[iAssignLoop][1];
		pPlanet->setBonusType(iBonus);
		//printd(" Assigned bonus to planet: bonus %d, system (%d,%d); planet ring %d (%s)" % (iBonus, pPlanet.getX(), pPlanet.getY(), pPlanet.getOrbitRing(), pPlanet.getName()))
		CvPlot* pPlot = plot(pPlanet->getX(), pPlanet->getY());
		pPlot->setBonusType((BonusTypes)iBonus);
		
	}
}

FeatureTypes CvMap::getFeatureIDSolarSystem()
{
	if (m_iFeatureIDSolarSystem <=0 || m_iFeatureIDSolarSystem >= GC.getNumFeatureInfos())//needs initialization
	{
		m_iFeatureIDSolarSystem = (FeatureTypes)GC.getInfoTypeForString("FEATURE_SOLAR_SYSTEM"); 
	}
	return m_iFeatureIDSolarSystem;
}
int CvMap::getPlanetRange2() const
{
	return m_g_iPlanetRange2;
}
int CvMap::getPlanetRange3() const
{
	return m_g_iPlanetRange3;
}

void CvMap::generateSolarSystems()
{
		//solar system generation - they have to know starting plots and cities
	for (int il = 0; il < GC.getMapINLINE().numPlotsINLINE();il++)
	{
		CvPlot* pPlot= GC.getMapINLINE().plotByIndexINLINE(il);
		if (pPlot->getFeatureType() == (FeatureTypes)GC.getInfoTypeForString("FEATURE_SOLAR_SYSTEM"))
			{
				CvSolarSystem* pSystem = createRandomSystem(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_YIELD, 3);
				//FAssertMsg(false, "report 2");
				int ID = getFreeSolarSystemIndex();
				if (ID==-1)
				{
					growSolarSystemsTable();
					ID = getFreeSolarSystemIndex();
				}
				FAssertMsg(ID>-1, "report 3");
				pSystem->setID(ID);
				//addSolarSystem();
				setSolarSystem(pSystem, ID);
				pPlot->setPlotSolarSystemID(ID);
				if (pPlot->isCity())
				{
					CvCity* pCity = pPlot->getPlotCity();
					pSystem->setCity(pCity->getIDInfo());
					pCity->setCitySolarSystem(ID);
				}
				//FAssertMsg(false, "report 4");
			}
	}
	AddPlanetaryResources();
}
//Aradziem end
//////////////////////////////////////////////////////////////////////////
// Protected Functions...
//////////////////////////////////////////////////////////////////////////

void CvMap::calculateAreas()
{
	PROFILE("CvMap::calculateAreas");
	CvPlot* pLoopPlot;
	CvArea* pArea;
	int iArea;
	int iI;

	for (iI = 0; iI < numPlotsINLINE(); iI++)
	{
		pLoopPlot = plotByIndexINLINE(iI);
		gDLL->callUpdater();
		FAssertMsg(pLoopPlot != NULL, "LoopPlot is not assigned a valid value");

		if (pLoopPlot->getArea() == FFreeList::INVALID_INDEX)
		{
			pArea = addArea();
			pArea->init(pArea->getID(), pLoopPlot->isWater());

			iArea = pArea->getID();

			pLoopPlot->setArea(iArea);

			gDLL->getFAStarIFace()->GeneratePath(&GC.getAreaFinder(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), -1, -1, pLoopPlot->isWater(), iArea);
		}
	}
}




// Private Functions...

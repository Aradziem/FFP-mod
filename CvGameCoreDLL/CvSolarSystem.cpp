//CvSolarSystem.cpp
//Aradziem
//rewrite solar system stuff from python here

#include "CvGameCoreDLL.h"
#include "CvSolarSystem.h"
//#include "CvCity.h"
//#include "CvGlobals.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvGameCoreUtils.h"

// Public Functions...
CvSolarSystem::CvSolarSystem(int iX,int iY, bool bIsStarbase, StarTypes iSunType)
{
	m_bNeedsUpdate = false;
	m_iX=iX;
	m_iY=iY;

	m_bIsStarbase = bIsStarbase;

	m_iStarType=iSunType;

	m_iSelectedPlanet = -1;
	m_iBuildingPlanetRing = -1;
		
	m_iSingleBuildingLocations = 0;
	m_aaiSingleBuildingLocations = NULL;
		
	m_iNumPlanets = 0;
	m_apPlanets = NULL;
	//CvDLLEntity::createSolarSystemEntity(this);		// create and attach entity to city

	m_piYields = new int[NUM_YIELD_TYPES];
	for (int i = 0; i < NUM_YIELD_TYPES; i++)
	{
		m_piYields[i] = 0;
	}
}

CvSolarSystem::~CvSolarSystem()
{
	//CvDLLEntity::removeEntity();			// remove entity from engine
	//CvDLLEntity::destroyEntity();			// delete CvCityEntity and detach from us

	SAFE_DELETE_ARRAY(m_aaiSingleBuildingLocations);
	SAFE_DELETE_ARRAY(m_apPlanets);
	SAFE_DELETE_ARRAY(m_piYields);
}

void CvSolarSystem::reset(int iX, int iY, bool bIsStarbase, StarTypes iStarType)
{
	m_bNeedsUpdate = false;
	m_iX = iX;
	m_iY = iY;

	m_bIsStarbase = bIsStarbase;
	m_iStarType = iStarType;

	m_iSelectedPlanet = -1;
	m_iBuildingPlanetRing = -1;

	m_iSingleBuildingLocations = 0;
	m_aaiSingleBuildingLocations = NULL;

	m_iNumPlanets = 0;
	m_apPlanets = NULL;
}

void setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
	{
		return;
	}

	//CvDLLEntity::setup();

	//these could be useful:
	//setInfoDirty(true);
	//setLayoutDirty(true);
}

void CvSolarSystem::setCity(IDInfo pCity)
{
	m_pCity = pCity ;
}

bool CvSolarSystem::setCity()
{
	CvPlot* pPlot = GC.getMapINLINE().plot(getX(), getY());
	//CvCity* pCity;
	if (pPlot->isCity())
	{
		m_pCity = pPlot->getPlotCity()->getIDInfo();
		return true;
	}
	return false;
}

void CvSolarSystem::updateYield()
{
	int iNewYield = 0;
	int iOldYield = 0;
	
	CvCity* pCity = getCity();
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		iNewYield = 0;
		iOldYield = m_piYields[iI];
		for (int i = 0;i < m_iNumPlanets;i++)
		{
			CvPlanet* pPlanet = getPlanetByIndex(i);
			iNewYield = iNewYield + pPlanet->getTotalYield(getOwner(), (YieldTypes)iI);
		}
		if (pCity)
		{
			pCity->changeBaseYieldRate(((YieldTypes)iI), (iNewYield - iOldYield));
			pCity->AI_setAssignWorkDirty(true);
		}
	}
}

bool CvSolarSystem::isNeedsUpdate() const
{
	return m_bNeedsUpdate;
}
void CvSolarSystem::setNeedsUpdate(bool bValue)
{
	m_bNeedsUpdate = bValue;
}

PlayerTypes CvSolarSystem::getOwner() const
{		
	return getCity()->getOwner();
}

CvCity* CvSolarSystem::getCity() const
{
	return ::getCity(m_pCity);
	/*CvPlot* pPlot = GC.getMapINLINE().plot(getX(), getY());
	//CvCity* pCity;
	if (pPlot->isCity())
	{
		return pPlot->getPlotCity();
	}
	return NULL;*/
}

int CvSolarSystem::getPopulation()
{		
	int iPop = 0;
		
	for(int iPlanetLoop = 0;iPlanetLoop < getNumPlanets();iPlanetLoop++)
	{
		iPop += getPlanetByIndex(iPlanetLoop)->getPopulation();
	}
	return iPop;
}
		
int CvSolarSystem::getX() const
{
	return m_iX;
}
	
int CvSolarSystem::getY() const
{
	return m_iY;
}

int CvSolarSystem::getID() const
{
	return m_ID;
}
void CvSolarSystem::setID(int newID)
{
	m_ID = newID;
}

bool CvSolarSystem::getIsStarbase() const
{
	return m_bIsStarbase;
}
void CvSolarSystem::setIsStarbase(bool bNewValue)
{
	m_bIsStarbase = bNewValue;
}

StarTypes CvSolarSystem::getStarType() const
{
	return m_iStarType;
}
void CvSolarSystem::setStarType(StarTypes iStarType)
{
	m_iStarType = iStarType;
}
		
int CvSolarSystem::getSelectedPlanet() const
{
	return m_iSelectedPlanet;
}

void CvSolarSystem::setSelectedPlanet(int iID)
{
	FAssert(iID>-1 && iID < m_iNumPlanets)
	m_iSelectedPlanet = iID;
		
	if (gDLL->getInterfaceIFace()->isCityScreenUp())
	{
			gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
	}
}	

int CvSolarSystem::getBuildingPlanetRing() const
{
	return m_iBuildingPlanetRing;
}

void CvSolarSystem::setBuildingPlanetRing(int iID)
{
	/*
	# Planet Production Memory Improvement - GE, March-2011
	# This does a lot more work now.
	# 1) Before changing build planet, save the production spent on each building
	#    type from the city's data to the pre-change planet's data
	# 2) After changing build planet, load the new planet's saved building production
	#    values into the city's building production list
	*/
	
	int iOldBuildingPlanetRing = m_iBuildingPlanetRing;
	//printd("setBuildingPlanet to %d from %d" %(iID, iOldBuildingPlanetRing))

	int iBuildingPlanetRing = iID;
		
	CvCity* pCity = getCity();
	if ((pCity == NULL) || (iOldBuildingPlanetRing == -1))
	{
		//# we are doing map setup so none of the following stuff is relevant
		return;
	}

	int iNumBuildingTypes = GC.getNumBuildingInfos();
	CvPlanet* pPlanet = getPlanet(iOldBuildingPlanetRing); //# pre-change planet
	for (int iBuilding = 0;iBuilding < iNumBuildingTypes; iBuilding++)
	{
		pPlanet->setBuildingProduction(iBuilding, pCity->getBuildingProduction((BuildingTypes)iBuilding ));
		if (pCity->getBuildingProduction((BuildingTypes)iBuilding) != 0)
		{
			//printd("  saved production for building: type = %d, production = %d" %(iBuilding, pCity.getBuildingProduction(iBuilding)))
		}
	}
	pPlanet = getPlanet(iID);// # post-change planet
	//printd("  Adjusting stored production values for the city based on new build planet's data")
	for (int iBuilding = 0;iBuilding < iNumBuildingTypes;iBuilding++)
	{
		pCity->setBuildingProduction((BuildingTypes)iBuilding, pPlanet->getBuildingProduction(iBuilding));
		if (pPlanet->getBuildingProduction(iBuilding))
		{
			//printd("    building type %d set to %d" %(iBuilding, pPlanet.getBuildingProduction(iBuilding)))
		}
	}
	if (gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
	}
}

int** CvSolarSystem::getSingleBuildingLocations()
{
	return m_aaiSingleBuildingLocations;
}


int CvSolarSystem::getSingleBuildingRingLocationByType(int iBuildingType)
{
	int iRingID = -1;

	for (int aiSingleBuildingLocation = 0;aiSingleBuildingLocation < m_iSingleBuildingLocations;aiSingleBuildingLocation++)
	{
		//iBuildingType = m_aaiSingleBuildingLocations[aiSingleBuildingLocation][0];
		iRingID = m_aaiSingleBuildingLocations[aiSingleBuildingLocation][1];
	}
	if (iRingID == -1)
	{
		//fassert
	}
	
	return iRingID;
}
		
void CvSolarSystem::setSingleBuildingRingLocation(int iBuildingType)
{		
		//# See if there are any other rings occupied
	int* aiRingsOccupied = NULL;
	int len = 0;
		
	for (int aiSingleBuildingLocation = 0;aiSingleBuildingLocation < m_iSingleBuildingLocations; aiSingleBuildingLocation++)
	{
		len++;
		int iRingID = m_aaiSingleBuildingLocations[aiSingleBuildingLocation][1];
		int* temp = new int[len];
		for(int i=0;i<len-1;i++)
		{
			temp[i] = aiRingsOccupied[i];
		}
		temp[len - 1] = iRingID;
		aiRingsOccupied = temp;
	}
		
	int* aiFreeRings = NULL;
	len = 0;
	for (int iRingLoop = 0;iRingLoop < 9;iRingLoop++)// Orbit rings 1 through 8
	{
		bool in = false;
		for (int i = 0;i < len;i++)
		{
			if (iRingLoop == aiRingsOccupied[i])
			{
				in = true;
				break;
			}
		}
		if(!in)
		{
			len++;
			int* temp = new int[len];
			for (int i = 0;i < len - 1;i++)
			{
				temp[i] = aiFreeRings[i];
			}
			temp[len - 1] = iRingLoop;
			aiFreeRings = temp;
		}
	}
	//# Pick a ring at random
	int iRand = GC.getGameINLINE().getSorenRandNum(len, "Picking random ring to attach single building to");
	int iRing = aiFreeRings[iRand];

	//#XML override (use iRing = 0 to avoid cluttering the real rings, for the Star Fortress)
	int iXMLOverride = GC.getBuildingInfo((BuildingTypes)iBuildingType).getSingleRingBuildingLocation();
	if (iXMLOverride != -1)
	{
		iRing = 0;
	}

	//# Add entry to the array
	m_iSingleBuildingLocations += 1;
	int** temp = new int* [m_iSingleBuildingLocations];
	for (int i = 0;i < m_iSingleBuildingLocations - 1;i++)
	{
		temp[i] = m_aaiSingleBuildingLocations[i];
	}
	temp[m_iSingleBuildingLocations - 1] = new int[iBuildingType, iRing];
	m_aaiSingleBuildingLocations = temp;
}

		
int CvSolarSystem::getNumPlanets() const
{
	return m_iNumPlanets;
}
		
CvPlanet* CvSolarSystem::getPlanetByIndex(int iID)
{
	return m_apPlanets[iID];
}

int CvSolarSystem::getPlanetIndexByRing(int ring)
{
	if (ring < 0)
	{
		return -1;
	}
	for (int i = 0; i < m_iNumPlanets;i++)
	{
		if (m_apPlanets[i]->getOrbitRing() == ring)
		{
			return i;
		}
	}
	return -1;
}


void CvSolarSystem::addPlanet(int iPlanetType, int iPlanetSize, int iOrbitRing, int bMoon, int bRings)
{

	//#printd("Adding planet to (%d, %d) system with: %s, %d, %d, %s, %s" %(self.getX(), self.getY(), aszPlanetTypeNames[iPlanetType], iPlanetSize, iOrbitRing, bMoon, bRings))

	bool bLoading = false;

	//# PlanetType of - 1 denotes this planet is being added through the load sequence
	if (iPlanetType == -1)
	{
		bLoading = true;
	}

	if (getNumPlanets() < 8 || bLoading == true)
	{

		/*# So... err...
			# Turns out that this code never checks to make sure there isn't already a planet in the ring.
			# More to the point, if there is a planet in the ring, it doesn't get deleted from the array.
			# For now, we'll just check to make sure getPlanet(iOrbitRing) == -1.
		*/
		if (m_iNumPlanets < iOrbitRing && getPlanet(iOrbitRing) != NULL)
		{
			return;
		}
		m_iNumPlanets++;
		CvPlanet** temp = new CvPlanet*[m_iNumPlanets];
		for (int i = 0;i < m_iNumPlanets-1;i++)
		{
			temp[i] = m_apPlanets[i];
		}
		temp[m_iNumPlanets - 1] = new CvPlanet(getX(), getY(),(PlanetTypes) iPlanetType,(PlanetSizes) iPlanetSize, iOrbitRing, bMoon, bRings);
		m_apPlanets = new CvPlanet*[m_iNumPlanets];
		for (int il = 0; il < m_iNumPlanets;il++)
		{
			m_apPlanets[il] = temp[il];
		}
		if (getBuildingPlanetRing() == -1)
		{
			setBuildingPlanetRing(iOrbitRing);
			if (bLoading == true) //# This is a block to prevent double - dipping on load of game
			{
				m_iNumPlanets--;
			}
		}
	}
}

CvPlanet* CvSolarSystem::getPlanet(int iRingID)
{	
	FAssert(iRingID >-1 || iRingID < 8, "MASSIVE FAILURE!!!!!!!!!!: Trying to access invalid Ring ID ");
	for (int iPlanetLoop = 0 ;iPlanetLoop < getNumPlanets();iPlanetLoop++)
	{
		CvPlanet* pPlanet = getPlanetByIndex(iPlanetLoop);
			
		if (pPlanet->getOrbitRing() == iRingID)
		{
			return pPlanet;
		}
	}		
	return NULL;
}

int CvSolarSystem::getPopulationLimit(bool bFactorHappiness)
{

	int iPlanetPopLimit = 0;
	int iHappyPopLimit = getCity()->getPopulation();

	//# Loop through all planets and get the sum of their pop limits
	for (int iPlanetLoop = 0;iPlanetLoop < getNumPlanets();iPlanetLoop++)
	{
		CvPlanet* pPlanet = getPlanetByIndex(iPlanetLoop);

		iPlanetPopLimit += pPlanet->getPopulationLimit(getOwner());
	}

	if (bFactorHappiness)
	{
		iHappyPopLimit -= getCity()->angryPopulation(0);
	}
	else
	{
		iHappyPopLimit = iPlanetPopLimit;
	}

	return std::min(iPlanetPopLimit, iHappyPopLimit);
}
	
int**  CvSolarSystem::getSizeYieldPlanetIndexList(YieldTypes iYield)
{
	/*
	# Get a list of planets in the system sorted(descending) by current population limit
	# as the main citeria  with planets of the same size sorted(descending) by the passed
	# yield value - planets with both the same are in indeterminant order(probably, but
	# not necessarily, in the order they appear in the system's planet list).
	# The returned value is the sorted list, each element in it is itself
	# a list(I would have used tuples, but everything else here uses lists)
	# holding the current population limit, the yield of the specified type, and the planet index.
	*/
	int** aiPlanetIndexList = new int*[getNumPlanets()];

	//# Loop through all planets
	for (int iPlanetLoop = 0;iPlanetLoop < getNumPlanets();iPlanetLoop++)
	{

		CvPlanet* pPlanet = getPlanetByIndex(iPlanetLoop);
		aiPlanetIndexList[iPlanetLoop] = new int[pPlanet->getPopulationLimit(getOwner()), pPlanet->getTotalYield(getOwner(), iYield), iPlanetLoop];
	}
	bool bSorted = false;
	int* temp;
	while (bSorted != true)
	{
		bSorted = true;
		for (int i = 0; i < getNumPlanets()-1;i++)
		{
			if (aiPlanetIndexList[i][0] < aiPlanetIndexList[i + 1][0])
			{
				bSorted = false;
				temp = aiPlanetIndexList[i];
				aiPlanetIndexList[i] = aiPlanetIndexList[i + 1];
				aiPlanetIndexList[i + 1] = aiPlanetIndexList[i];
			}

		}
	}

	return aiPlanetIndexList;
}

void CvSolarSystem::read(FDataStreamBase* pStream)
{
	reset();

	//uint uiFlag = 0;
	//pStream->Read(&uiFlag);
	pStream->Read(&m_ID);

	pStream->Read(&m_bNeedsUpdate);

	pStream->Read(&m_iX);
	pStream->Read(&m_iY);

	pStream->Read(&m_bIsStarbase);

	pStream->Read((int*)&m_iStarType);

	pStream->Read(&m_iSelectedPlanet);
	pStream->Read(&m_iBuildingPlanetRing);

	pStream->Read(NUM_YIELD_TYPES, m_piYields);

	pStream->Read(&m_iSingleBuildingLocations);
	for (int i = 0;i < m_iSingleBuildingLocations;i++)
	{
		pStream->Read(2, m_aaiSingleBuildingLocations[i]);
	}
	pStream->Read(&m_iNumPlanets);
	m_apPlanets = new CvPlanet * [m_iNumPlanets];
	for (int i = 0;i < m_iNumPlanets; i++)
	{
		m_apPlanets[i]->read(pStream);
	}

}

void CvSolarSystem::addBasicBuildingsToBestPlanet()
{

	CvPlot* pPlot = GC.getMapINLINE().plot(m_iX, m_iY);
	CvCity* pCity = pPlot->getPlotCity();

	//#Set "basic buildings" to what is defined in CivilizationInfos FreeBuildingClasses.
	int iBestPlanetIndex = getBestPlanetInSystem();
	CvPlanet* pPlanet = getPlanetByIndex(iBestPlanetIndex);
	CvCivilizationInfo& pCiv = GC.getCivilizationInfo(GET_PLAYER(pCity->getOwner()).getCivilizationType());
	for (int iBuildingClass = 0;iBuildingClass < GC.getNumBuildingClassInfos();iBuildingClass++)
	{
		if (iBuildingClass != GC.getInfoTypeForString("BUILDINGCLASS_CAPITOL"))
		{
			if (pCiv.isCivilizationFreeBuildingClass(iBuildingClass))
			{
				int iBuilding = pCiv.getCivilizationBuildings(iBuildingClass);
				pCity->setNumRealBuilding((BuildingTypes)iBuilding, pCity->getNumRealBuilding((BuildingTypes)iBuilding) + 1);
				pPlanet->setHasBuilding(iBuilding, true);
			}
		}
	}
	//CvCity* testCity = pPlot->getPlotCity();
}

int CvSolarSystem::getBestPlanetInSystem()
{
	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(m_iX, m_iY);
	CvCity* pCity;
	PlayerTypes	iOwner = NO_PLAYER;
	if (pPlot->isCity())
	{
		pCity = pPlot->getPlotCity();
		if (pCity != NULL)
		{
			PlayerTypes	iOwner = pCity->getOwner();
		}
	}
	
	
	//# Loop through all planets to find the best one(we'll call it our homeworld)
	int iBestPlanetValue = -2;//# DarkLunaPhantom - It was - 1.
	int iBestPlanetIndex = -1;
	for (int iPlanetLoop = 0;iPlanetLoop < getNumPlanets();iPlanetLoop++)
	{
		CvPlanet* pPlanet = getPlanetByIndex(iPlanetLoop);

		//#iValue = (pPlanet.getBaseYield(0) * 2) + pPlanet.getBaseYield(1) (Food * 2) + Production
		//#Changed method to Treaciv's system: TC01
		int iValue = ((pPlanet->getBaseYield((YieldTypes)0) * 150) + (pPlanet->getBaseYield((YieldTypes)1) * 35) + (pPlanet->getBaseYield((YieldTypes)2) * 10));
		iValue *= (pPlanet->getPlanetSize() + 8);

		//# Planet is unusable, and therefore pretty much worthless to us
		if (pPlanet->getPopulationLimit(iOwner) == 0)
		{
			iValue = 0;
		}
		//# DarkLunaPhantom
		if (pPlanet->isDisabled())
		{
			iValue = -1;
		}

		if (iValue > iBestPlanetValue)
		{
			iBestPlanetValue = iValue;
			iBestPlanetIndex = iPlanetLoop;
		}
	}
	return iBestPlanetIndex;
}


void CvSolarSystem::write(FDataStreamBase* pStream)
{
	//uint uiFlag = 0;
	//pStream->Read(&uiFlag);

	pStream->Write(&m_ID);

	pStream->Write(&m_bNeedsUpdate);

	pStream->Write(&m_iX);
	pStream->Write(&m_iY);

	pStream->Write(&m_bIsStarbase);

	pStream->Write(&m_iStarType);

	pStream->Write(&m_iSelectedPlanet);
	pStream->Write(&m_iBuildingPlanetRing);

	pStream->Write(NUM_YIELD_TYPES, m_piYields);

	pStream->Write(&m_iSingleBuildingLocations);
	for (int i = 0;i < m_iSingleBuildingLocations;i++)
	{
		pStream->Write(2, m_aaiSingleBuildingLocations[i]);
	}
	pStream->Write(&m_iNumPlanets);
	for (int i = 0;i < m_iNumPlanets; i++)
	{
		m_apPlanets[i]->write(pStream);
	}
}

/*
	check write and read functions
	def getData(self):
		
		aArray = []
		
		aArray.append(self.getSunType())		# 0
		aArray.append(self.getSelectedPlanet())	# 1
		aArray.append(self.getBuildingPlanetRing())	# 2
		aArray.append(self.getNumPlanets())	# 3
		
		for iPlanetLoop in range(self.getNumPlanets()):
			pPlanet = self.getPlanetByIndex(iPlanetLoop)
			aArray.append(pPlanet.getData())	# 4 through X
			#printd("Saving Planet at Ring %d" %(pPlanet.getOrbitRing()))
		
		aArray.append(self.aaiSingleBuildingLocations)		# Can be of any size
		
		return aArray
		
	def setData(self, aArray):
		
		iIterator = 0
		
		self.iSunType				= aArray[iIterator]
		iIterator += 1
		self.iSelectedPlanet		= aArray[iIterator]
		iIterator += 1
		self.iBuildingPlanetRing	= aArray[iIterator]
		iIterator += 1
		self.iNumPlanets			= aArray[iIterator]
		iIterator += 1
		
		for iPlanetLoop in range(self.getNumPlanets()):
			#printd("Adding planet ID %d" %(iPlanetLoop))
			self.addPlanet()
			pPlanet = self.getPlanetByIndex(iPlanetLoop)
			pPlanet.setData(aArray[iIterator])	# 4 through X
			iIterator += 1
			#printd("Loading Planet at Ring %d" %(pPlanet.getOrbitRing()))
			
		self.aaiSingleBuildingLocations = aArray[iIterator]		# Can be of any size
*/

void CvSolarSystem::updateDisplay()
{
	
		
	//#printd("Updating display at %d, %d" %(self.getX(), self.getY()))
		
	CvPlot* pPlot = GC.getMapINLINE().plot(m_iX, m_iY);
	//FeatureTypes test = GC.getMap().getFeatureIDSolarSystem();
	if(pPlot->getFeatureType() != GC.getMap().getFeatureIDSolarSystem() && !m_bIsStarbase)
	{
		pPlot->setFeatureType(GC.getMap().getFeatureIDSolarSystem(), 0);
	}
	pPlot->resetFeatureModel();
	for (int iOrbitLoop = 1;iOrbitLoop < 9;iOrbitLoop++)
	{
		CvString str =  "FEATURE_DUMMY_TAG_ORBIT_"+CvString::format("%d", iOrbitLoop);
		pPlot->setFeatureDummyVisibility(str.GetCString(), false);
	}
		//#printd("\nAdding Sun at %d, %d" %(pPlot.getX(), pPlot.getY()))
	pPlot->addFeatureDummyModel("FEATURE_DUMMY_TAG_SUN",GC.getStarInfo(getStarType()).getArtDefineTag());
	//pPlot->addFeatureDummyModel("FEATURE_DUMMY_TAG_SUN",GC.getMap().getSunTypes()[getSunType()]);
		
	if (pPlot->isRevealed(GC.getGameINLINE().getActiveTeam(), false))
	{
		//# Only displayed when plot is visible to active player
		if (pPlot->isVisible(GC.getGameINLINE().getActiveTeam(), false))
		{
				
			//# Add on-map Star System-based buildings
			int** aiArray = getSingleBuildingLocations();
			CvString szBuildingString ;

			for (int aiBuildingLocation = 0 ; aiBuildingLocation < m_iSingleBuildingLocations ; aiBuildingLocation++)
			{
					int iBuildingType = aiArray[aiBuildingLocation][0];
					int iRing = aiArray[aiBuildingLocation][1];
					int iXMLOverride = GC.getBuildingInfo((BuildingTypes)iBuildingType).getSingleRingBuildingLocation();
					if ( iXMLOverride != -1)
					{
						//check whether it works
						CvWString text = "FEATURE_DUMMY_TAG_BUILDING_" + CvWString::format(L"%d", iXMLOverride);
						int len=text.length();
						char* szBuildingString = new char[len];
						for(int i=0;i<len;i++)
						{
							szBuildingString[i]=text[i];
						}
					}
					else
					{
						szBuildingString = GC.getOrbitInfo((OrbitTypes)iRing).getBuildingArtDefineTag();
					}

					pPlot->addFeatureDummyModel(szBuildingString,GC.getBuildingInfo((BuildingTypes)iBuildingType).getSystemArtTag());
					
			}
				
			//# Astral Gate
			if (pPlot->isCity())
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (pCity->isCapital())
				{
					TeamTypes iTeam = pCity->getTeam();
					ProjectTypes iProjectAstralGate =(ProjectTypes) GC.getInfoTypeForString("PROJECT_ASTRAL_GATE");
					int iNumGatePieces = int(GET_TEAM(iTeam).getProjectCount((ProjectTypes)iProjectAstralGate) / (1 + 0.5 * (GET_TEAM(iTeam).getNumMembers() - 1)));
					if (iNumGatePieces > 0)
					{
						int iNumStages = GC.getProjectInfo(iProjectAstralGate).getNumStageArtDefineTags();
						int* stages = GC.getProjectInfo(iProjectAstralGate).getStageForArtDefineTag();
						CvString szBuilding;
						if (stages[iNumStages - 1] < iNumGatePieces)
						{
							szBuilding = GC.getProjectInfo(iProjectAstralGate).getStageArtDefineTags()[iNumStages - 1];
						}
						else
						{
							for (int i = 0;i < iNumStages;i++)
							{
								if(stages[i] = iNumGatePieces)
								{
									szBuilding = GC.getProjectInfo(iProjectAstralGate).getStageArtDefineTags()[i];
									break;
								}
								else if(stages[i] > iNumGatePieces)
								{
									szBuilding = GC.getProjectInfo(iProjectAstralGate).getStageArtDefineTags()[i - 1];
									break;
								}
							}
						}
						szBuildingString = "FEATURE_DUMMY_TAG_BUILDING_9";
						pPlot->addFeatureDummyModel(szBuildingString, szBuilding);
					}
				}
			}
		}
	}
	
	for (int iPlanetLoop = 0;iPlanetLoop < getNumPlanets(); iPlanetLoop++)
	{

		//#printd("Planet %d" % (iPlanetLoop))
		CvPlanet* pPlanet = getPlanetByIndex(iPlanetLoop);

		//#printd("Planet: %s, Size: %d, Ring: %d, %s, %s" % (aszPlanetTypeNames[pPlanet.getPlanetType()], pPlanet.getPlanetSize(), pPlanet.getOrbitRing(), pPlanet.isMoon(), pPlanet.isRings()))

		OrbitTypes OrbitID = (OrbitTypes)pPlanet->getOrbitRing();
		CvString szOrbitString = GC.getOrbitInfo(OrbitID).getOrbitArtDefineTag();
		CvString szPlanetString = GC.getOrbitInfo(OrbitID).getPlanetArtDefineTag();
		CvString szPlanetTexture = GC.getPlanetInfo(pPlanet->getPlanetType()).getArtDefineTag();
		CvString szPlanetSize = GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizeArtTag();

		pPlot->setFeatureDummyVisibility(szOrbitString, true);
		pPlot->addFeatureDummyModel(szPlanetString, szPlanetSize);
		pPlot->setFeatureDummyTexture(szPlanetString, szPlanetTexture);

		//# Disabled by Doomsday Missile ?
		if (pPlanet->isDisabled())
		{
			pPlot->addFeatureDummyModel(szPlanetString, "FEATURE_MODEL_TAG_SUN_ORANGE");
		}
		else
		{
			PlanetTypes iType = pPlanet->getPlanetType();
			int iNumBaseFeatures = GC.getPlanetInfo(iType).getNumBaseFeatures();
			if (iNumBaseFeatures)
			{
				for (int i = 0;i < iNumBaseFeatures;i++)
				{
					int iFeatureType = GC.getPlanetInfo(iType).getBaseFeature(i);
					pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizeArtTags(iFeatureType));
				}
			}

			//# Rings effect
			if (pPlanet->isRings())
			{
				int iRingsEffect = GC.getInfoTypeForString("ART_TAG_TYPE_RINGS");
				pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizeArtTags(iRingsEffect));									
			}
			//# Moon orig art
			if (pPlanet->isMoon())
			{
				int iMoonEffect = GC.getInfoTypeForString("ART_TAG_TYPE_MOON");
				pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizeArtTags(iMoonEffect));
			}

			//# Only displayed when plot is visible to active player
			if (pPlot->isVisible(GC.getGameINLINE().getActiveTeam(), false))
			{
				//# Population Lights effect
				if (pPlanet->getPopulation() >= 1)
				{
					int iNumPopArtTags = GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getNumPlanetSizePopulationArtTagTypes();
					int* iPopNums = GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizePopulationForArtTags();
					if (pPlanet->getPopulation() >= iPopNums[iNumPopArtTags - 1])
					{
						pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizePopulationArtTags()[iNumPopArtTags - 1]);
					}
					else
					{
						for (int i = 0; i < iNumPopArtTags - 1;i++)
						{
							if (pPlanet->getPopulation() == iPopNums[i])
							{
								pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizePopulationArtTags()[i]);
							}
							else if (pPlanet->getPopulation() <= iPopNums[i])
							{
								pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizePopulationArtTags()[i-1]);
							}
						}
					}
				}
				//# Buildings
				for (int i = 0;i < GC.getNumBuildingInfos();i++)
				{
					if (pPlanet->isHasBuilding(i))
					{
						if (GC.getBuildingInfo((BuildingTypes)i).isArtTagSizeDependent())
						{
							pPlot->addFeatureDummyModel(szPlanetString, GC.getBuildingInfo((BuildingTypes)i).getPlanetSizeArtTag(pPlanet->getPlanetSize()));
						}
					}
				}
				//# Selected ?
				if (gDLL->getInterfaceIFace()->isCityScreenUp())
				{
					if (getSelectedPlanet() == getPlanetIndexByRing(pPlanet->getOrbitRing()))
					{
						int iSelectEffect = GC.getInfoTypeForString("ART_TAG_TYPE_SELECTION");
						pPlot->addFeatureDummyModel(szPlanetString, GC.getPlanetSizeInfo(pPlanet->getPlanetSize()).getPlanetSizeArtTags(iSelectEffect));
					}
				}
			}
		}
	}
	
}

// Public Functions...

/*CvPlanet::CvPlanet()
{
	m_szName = "";
	
	m_iX = -1;
	m_iY = -1;
	
	m_iPlanetType = -1;
	m_iPlanetSize = -1;
	m_iOrbitRing = -1;
	m_bMoon = false;
	m_bRings = false;
	
	m_iPopulation = 0;
	
	m_bDisabled = false;
		
	m_iBonus = -1;
		
	m_abHasBuilding = NULL;
	m_aiBuildingProduction = NULL;
}*/

CvPlanet::CvPlanet(int iX , int iY , PlanetTypes iPlanetType, PlanetSizes iPlanetSize, int iOrbitRing, bool bMoon, bool bRings)
{
		m_szName = "";
		
		m_iX = iX;
		m_iY = iY;
		
		m_iPlanetType = iPlanetType;
		m_iPlanetSize = iPlanetSize;
		m_iOrbitRing = iOrbitRing;
		m_bMoon = bMoon;
		m_bRings = bRings;
		
		m_iPopulation = 0;
		
		m_bDisabled = false;
		
		m_iBonus = -1;
		int i_buildingInfos = GC.getNumBuildingInfos();
		m_abHasBuilding = new bool[i_buildingInfos];

		for(int iBuildingLoop = 0;iBuildingLoop < i_buildingInfos;iBuildingLoop++)
		{
			m_abHasBuilding[iBuildingLoop]=false;
		}
		m_aiBuildingProduction = new int[i_buildingInfos];
		for(int iBuildingLoop = 0; iBuildingLoop < i_buildingInfos; iBuildingLoop++)
		{
			m_aiBuildingProduction[iBuildingLoop]=0;
		}
}

CvPlanet::~CvPlanet()
{
	SAFE_DELETE_ARRAY(m_aiBuildingProduction);
	SAFE_DELETE_ARRAY(m_abHasBuilding);
}

void CvPlanet::reset(int iX, int iY, PlanetTypes iPlanetType, PlanetSizes iPlanetSize, int iOrbitRing, bool bMoon, bool bRings)
{
	m_szName = "";

	m_iX = iX;
	m_iY = iY;

	m_iPlanetType = iPlanetType;
	m_iPlanetSize = iPlanetSize;
	m_iOrbitRing = iOrbitRing;
	m_bMoon = bMoon;
	m_bRings = bRings;

	m_iPopulation = 0;

	m_bDisabled = false;

	m_iBonus = -1;
	int i_buildingInfos = GC.getNumBuildingInfos();
	m_abHasBuilding = new bool[i_buildingInfos];

	for (int iBuildingLoop = 0;iBuildingLoop < i_buildingInfos;iBuildingLoop++)
	{
		m_abHasBuilding[iBuildingLoop] = false;
	}
	m_aiBuildingProduction = new int[i_buildingInfos];
	for (int iBuildingLoop = 0; iBuildingLoop < i_buildingInfos; iBuildingLoop++)
	{
		m_aiBuildingProduction[iBuildingLoop] = 0;
	}
}

CvWString CvPlanet::getName()
{
	return m_szName;
}

void CvPlanet::setName(CvWString szName)
{
	m_szName = szName;
}

int CvPlanet::getX() const
{
	return m_iX;
}

int CvPlanet::getY() const
{
	return m_iY;
}
		
PlanetTypes CvPlanet::getPlanetType() const
{
	return m_iPlanetType;
}
		
PlanetSizes CvPlanet::getPlanetSize() const
{
	return m_iPlanetSize;
}

int CvPlanet::getOrbitRing() const
{
	return m_iOrbitRing;
}

bool CvPlanet::isMoon() const
{
	return m_bMoon;
}

bool CvPlanet::isRings() const
{
	return m_bRings;
}

bool CvPlanet::isPlanetWithinCulturalRange() const
{
		
	//# Culture influences what Planets can be worked
	CvPlot* pPlot = GC.getMapINLINE().plot(getX(), getY());
		
	if (pPlot->isCity())
	{
		CvCity* pCity = pPlot->getPlotCity();
			
		//# Third ring
		if (getOrbitRing() >= GC.getMap().getPlanetRange3())
		{
			if (pCity->getCultureLevel() < 3)
			{
				return false;
			}
		}
		//# Second ring
		else if (getOrbitRing() >= GC.getMap().getPlanetRange2())
		{
			if (pCity->getCultureLevel() < 2)
			{
				return false;
			}
		}	
	return true;
	}
}
	
int CvPlanet::getPlanetCulturalRange() const
{
	//# Same as isPlanetWithinCulturalRange but returns a number
	CvPlot* pPlot = GC.getMapINLINE().plot(getX(),getY());
		
	if (pPlot->isCity())
	{
		CvCity* pCity = pPlot->getPlotCity();
			
		//# Third ring
		if (getOrbitRing() >= GC.getMap().getPlanetRange3())
		{
			return 2;
		}

		//# Second ring
		else if (getOrbitRing() >= GC.getMap().getPlanetRange2())
		{
			return 1;
		}
	}
	return 0;
}

int CvPlanet::getPopulation() const
{
	return m_iPopulation;
}

void CvPlanet::setPopulation(int iValue)
{
	m_iPopulation = iValue;
}

void CvPlanet::changePopulation(int iChange)
{
	m_iPopulation += iChange;
}
		
int CvPlanet::getPopulationLimit(PlayerTypes iOwner)
{
	if(isDisabled())
	{
		return 0;
	}

	if (!isPlanetWithinCulturalRange())
	{
		return 0;
	}

	int iMaxPop = getPlanetSize() + 1;
		
	//#Population cap increases from buildings
	for (int iBuildingLoop = 0;iBuildingLoop < GC.getNumBuildingInfos();iBuildingLoop++)
	{
		if (isHasBuilding(iBuildingLoop))
		{
			iMaxPop += GC.getBuildingInfo((BuildingTypes)iBuildingLoop).getPlanetPopCapIncrease();
		}
	}
		//#Population cap increases from civics
	if (iOwner > -1)
	{
		for (int iCivicOptionLoop = 0 ;iCivicOptionLoop < GC.getNumCivicOptionInfos();iCivicOptionLoop++)
		{
			for (int iCivicLoop = 0; iCivicLoop < GC.getNumCivicInfos();iCivicLoop++)
			{
				if (GET_PLAYER(iOwner).getCivics((CivicOptionTypes)iCivicOptionLoop) == iCivicLoop)
				{
					iMaxPop += GC.getCivicInfo((CivicTypes)iCivicLoop).getPlanetPopCapIncrease();
				}
			}
		}
	}

	return iMaxPop;
}
		
bool CvPlanet::isDisabled() const
{
	return m_bDisabled;
}

void CvPlanet::setDisabled(bool bValue)
{
	m_bDisabled = bValue;
}
		
void CvPlanet::setBonusType(int iBonusType)
{
	m_iBonus = iBonusType;
}

int CvPlanet::getBonusType() const
{
	return m_iBonus;
}

bool CvPlanet::isBonus() const
{
	return (m_iBonus != -1);
}

bool CvPlanet::isHasBonus(int iBonusType) const
{
	return (m_iBonus == iBonusType);
}
		
bool CvPlanet::isHasBuilding(int iID)
{
	return m_abHasBuilding[iID];
}

void CvPlanet::setHasBuilding(int iID, bool bValue)
{
	m_abHasBuilding[iID] = bValue;
}
		
int CvPlanet::getBuildingProduction(int iID)
{
	return m_aiBuildingProduction[iID];
}

void CvPlanet::setBuildingProduction(int iID,int iValue)
{
	m_aiBuildingProduction[iID] = iValue;
}
		
int CvPlanet::getTotalYield(int iOwner,YieldTypes iYieldID)
{
	return getBaseYield(iYieldID) + getExtraYield(iOwner, iYieldID);
}
		
int CvPlanet::getBaseYield(YieldTypes iYieldID) const
{
	
	int theYield = GC.getPlanetInfo(getPlanetType()).getYield(iYieldID);
	if( isBonus())
	{
		if ((GC.getBonusInfo((BonusTypes)getBonusType()).getHappiness() > 0) && (iYieldID == YIELD_COMMERCE))
		{
			theYield += 1;
		}
		else if ((GC.getBonusInfo((BonusTypes)getBonusType()).getHealth() > 0) && (iYieldID == YIELD_FOOD))
		{
			theYield += 1;
		}
	}
		return theYield;
}

int CvPlanet::getExtraYield(int iOwner, YieldTypes iYieldID)
{		
	if(iOwner == -1)
	{
		return 0;
	}
	PlayerTypes pPlayer = (PlayerTypes)iOwner;
	int iExtraYield = 0;
		
	//#Yields from buildings
	for (int iBuildingLoop = 0;iBuildingLoop < GC.getNumBuildingInfos();iBuildingLoop++)
	{
		if (isHasBuilding(iBuildingLoop))
		{
			int iYieldChange = GC.getBuildingInfo((BuildingTypes)iBuildingLoop).getPlanetYieldChanges(iYieldID);
				
			//#Yields from buildings that require a trait
			for (int iTraitLoop = 0;iTraitLoop < GC.getNumTraitInfos();iTraitLoop++)
			{
				if ( GET_PLAYER(pPlayer).hasTrait((TraitTypes)iTraitLoop))
				{
					iYieldChange += GC.getBuildingInfo((BuildingTypes)iBuildingLoop).getTraitPlanetYieldChange(iTraitLoop, iYieldID);
				}
			}
					
			iExtraYield += iYieldChange;
		}
	}

	//#Yields from civics
	for (int iCivicOption = 0; iCivicOption < GC.getNumCivicOptionInfos();iCivicOption++)
	{
		for (int iCivic = 0;iCivic < GC.getNumCivicInfos();iCivic++)
		{
			if (GET_PLAYER(pPlayer).getCivics((CivicOptionTypes)iCivicOption) == iCivic)
			{
				iExtraYield += GC.getCivicInfo((CivicTypes)iCivic).getPlanetYieldChanges(iYieldID);
			}
		}
	}
	//# CP - add golden age effect, thanks for the idea of doing it here go to TC01
	if (GET_PLAYER(pPlayer).isGoldenAge())
	{
		if ((getBaseYield(iYieldID) + iExtraYield) >= GC.getYieldInfo(iYieldID).getGoldenAgeYieldThreshold())
		{
			iExtraYield += GC.getYieldInfo(iYieldID).getGoldenAgeYield();
		}
	}
		return iExtraYield;
}
void CvPlanet::read(FDataStreamBase* pStream)
{
	reset();

	//uint uiFlag = 0;
	//pStream->Read(&uiFlag);

	pStream->ReadString(m_szName);

	pStream->Read(&m_iX);
	pStream->Read(&m_iY);

	pStream->Read((int*) & m_iPlanetType);
	pStream->Read((int*) & m_iPlanetSize);
	pStream->Read(&m_iOrbitRing);
	pStream->Read(&m_bMoon);
	pStream->Read(&m_bRings);

	pStream->Read(&m_iPopulation);

	pStream->Read(&m_bDisabled);

	pStream->Read(&m_iBonus);

	pStream->Read(GC.getNumBuildingInfos(),m_abHasBuilding);
	pStream->Read(GC.getNumBuildingInfos(),m_aiBuildingProduction);

}
void CvPlanet::write(FDataStreamBase* pStream)
{
	//uint uiFlag = 0;
	//pStream->Read(&uiFlag);

	pStream->WriteString(m_szName);

	pStream->Write(&m_iX);
	pStream->Write(&m_iY);

	pStream->Write(&m_iPlanetType);
	pStream->Write(&m_iPlanetSize);
	pStream->Write(&m_iOrbitRing);
	pStream->Write(&m_bMoon);
	pStream->Write(&m_bRings);

	pStream->Write(&m_iPopulation);

	pStream->Write(&m_bDisabled);

	pStream->Write(&m_iBonus);

	pStream->Write(GC.getNumBuildingInfos(), m_abHasBuilding);
	pStream->Write(GC.getNumBuildingInfos(), m_aiBuildingProduction);
}



#include "CvGameCoreDLL.h"
#include "CvSolarSystem.h"
#include "CySolarSystem.h"
//Aradziem
//CySolarSystem functions
CySolarSystem::CySolarSystem() : m_pSolarSystem(NULL)
{

}

CySolarSystem::CySolarSystem(CvSolarSystem* pSolarSystem) : m_pSolarSystem(pSolarSystem)
{

}

int CySolarSystem::getSolarSystemID()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getID();
	}
	return NULL;
}

int CySolarSystem::getSolarSystemX()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getX();
	}
	return NULL;
}

int CySolarSystem::getSolarSystemY()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getY();
	}
	return NULL;
}

bool CySolarSystem::getSolarSystemNeedsUpdate()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->isNeedsUpdate();
	}
}

void CySolarSystem::setSolarSystemNeedsUpdate(bool newValue)
{
	if (m_pSolarSystem)
	{
		m_pSolarSystem->setNeedsUpdate(newValue);
	}
}

void CySolarSystem::updateSolarSystemDisplay()
{
	if (m_pSolarSystem)
	{
		m_pSolarSystem->updateDisplay();
	}
}

int CySolarSystem::getSolarSystemNumPlanets()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getNumPlanets();
	}
	return NULL;
}

int CySolarSystem::getBuildingPlanetID()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getPlanetIndexByRing(m_pSolarSystem->getBuildingPlanetRing());
	}
	return NULL;
}

int CySolarSystem::getBuildingPlanetRing()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getBuildingPlanetRing();
	}
	return NULL;
}

void CySolarSystem::setBuildingPlanetRing(int Ring)
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->setBuildingPlanetRing(Ring);
	}
}

int CySolarSystem::getSelectedPlanetRing()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getPlanetByIndex(m_pSolarSystem->getSelectedPlanet())->getOrbitRing();
	}
	return NULL;
}

int CySolarSystem::getSelectedPlanetID()
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getSelectedPlanet();
	}
	return NULL;
}
void CySolarSystem::setSelectedPlanetRing(int Ring)
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->setSelectedPlanet(m_pSolarSystem->getPlanetIndexByRing(Ring));
	}
}

void  CySolarSystem::setSelectedPlanetID(int ID)
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->setSelectedPlanet(ID);
	}
}

int CySolarSystem::getPlanetIDByRing(int planetRing)
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getPlanetIndexByRing(planetRing);
	}
	return -1;
}

int CySolarSystem::getPopulationLimit(bool bFactorHappiness)
{
	if (m_pSolarSystem)
	{
		return m_pSolarSystem->getPopulationLimit(bFactorHappiness);
	}
	return -1;
}

CyPlanet* CySolarSystem::getPlanet(int iID)
{
	if (m_pSolarSystem)
	{
		return new CyPlanet(m_pSolarSystem->getPlanetByIndex(iID));
	}
	return NULL;
}

//CyPlanet functions
CyPlanet::CyPlanet() : m_pPlanet(NULL)
{

}

CyPlanet::CyPlanet(CvPlanet* pPlanet) : m_pPlanet(pPlanet)
{

}

int CyPlanet::getPlanetRing()
{
	if (m_pPlanet)
	{
		return m_pPlanet->getOrbitRing();
	}
	return -1;
}

int CyPlanet::getPlanetSize()
{
	if (m_pPlanet)
	{
		return (int)m_pPlanet->getPlanetSize();
	}
	return -1;
}

int CyPlanet::getPlanetType()
{
	if (m_pPlanet)
	{
		return (int)m_pPlanet->getPlanetType();
	}
	return -1;
}

int CyPlanet::getPlanetBonus()
{
	if (m_pPlanet)
	{
		return (int)m_pPlanet->getBonusType();
	}
	return -1;
}

bool CyPlanet::isPlanetBonus()
{
	if (m_pPlanet)
	{
		return m_pPlanet->isBonus();
	}
	return false;
}

std::wstring CyPlanet::getPlanetName()
{
	if (m_pPlanet)
	{
		return m_pPlanet->getName();
	}
}

int CyPlanet::getPlanetYield(int iOwner, int yieldType)
{
	if (m_pPlanet)
	{
		return (int)m_pPlanet->getTotalYield(iOwner, (YieldTypes)yieldType);
	}
	return -1;
}

int CyPlanet::getPlanetPopulation()
{
	if (m_pPlanet)
	{
		return (int)m_pPlanet->getPopulation();
	}
	return -1;
}

int CyPlanet::getPlanetPopulationLimit(int iOwner)
{
	if (m_pPlanet)
	{
		return m_pPlanet->getPopulationLimit((PlayerTypes)iOwner);
	}
	return -1;
}

bool CyPlanet::isPlanetDisabled()
{
	if (m_pPlanet)
	{
		return m_pPlanet->isDisabled();
	}
	return false;
}

bool CyPlanet::isPlanetRings()
{
	if (m_pPlanet)
	{
		return m_pPlanet->isRings();
	}
	return false;
}

bool CyPlanet::isPlanetMoon()
{
	if (m_pPlanet)
	{
		return m_pPlanet->isMoon();
	}
	return false;
}

bool CyPlanet::isPlanetBuilding(int buildingID)
{
	if (m_pPlanet)
	{
		return m_pPlanet->isHasBuilding(buildingID);
	}
	return false;
}

bool CyPlanet::isPlanetWithinCulturalRange()
{
	if (m_pPlanet)
	{
		return m_pPlanet->isPlanetWithinCulturalRange();
	}
	return false;
}

int CyPlanet::getPlanetCulturalRange()
{
	if (m_pPlanet)
	{
		return m_pPlanet->getPlanetCulturalRange();
	}
	return false;
}
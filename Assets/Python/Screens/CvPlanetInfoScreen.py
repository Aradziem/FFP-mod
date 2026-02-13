# Final Frontier
# Designed & Programmed by: Jon 'Trip' Shafer

## Sid Meier's Civilization 4
## Copyright Firaxis Games 2007

import CvScreenEnums
import CvUtil
from CvPythonExtensions import *
import CvEventInterface

from CvSolarSystem import *

ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()
gc = CyGlobalContext()

class CvPlanetInfoScreen:
	"Planet Info Screen"
	def __init__(self, iScreenID):
#		self.iScreenID = iScreenID
		
		self.X_SCREEN = 0
		self.Y_SCREEN = 0
		self.W_SCREEN = 1024
		self.H_SCREEN = 768
		
		self.W_MAIN_PANEL = 325
		self.H_MAIN_PANEL = 760
		self.X_MAIN_PANEL = (self.W_SCREEN - self.W_MAIN_PANEL) / 2
		self.Y_MAIN_PANEL = (self.H_SCREEN - self.H_MAIN_PANEL) / 2
		
		self.W_LEVEL_1_PANEL = self.W_MAIN_PANEL - 20
		self.H_LEVEL_1_PANEL = 250
		self.X_LEVEL_1_PANEL = self.X_MAIN_PANEL + 10
		self.Y_LEVEL_1_PANEL = self.Y_MAIN_PANEL + 10
		
#Final Frontier Plus Changes: TC01
#	Since now the sixth planet is part of ring three, we need to change the screen to match.
#	That should mean changing self.H_LEVEL_2_PANEL to be 167, and then self.H_LEVEL_3_PANEL to be 250

		self.W_LEVEL_2_PANEL = self.W_MAIN_PANEL - 20
		self.H_LEVEL_2_PANEL = 167	#250
		self.X_LEVEL_2_PANEL = self.X_MAIN_PANEL + 10
		self.Y_LEVEL_2_PANEL = self.Y_LEVEL_1_PANEL + self.H_LEVEL_1_PANEL + 10
		
		self.W_LEVEL_3_PANEL = self.W_MAIN_PANEL - 20
		self.H_LEVEL_3_PANEL = 250	#167
		self.X_LEVEL_3_PANEL = self.X_MAIN_PANEL + 10
		self.Y_LEVEL_3_PANEL = self.Y_LEVEL_2_PANEL + self.H_LEVEL_2_PANEL + 10
		
		self.W_EXIT = 100
		self.H_EXIT = 30
		self.X_EXIT = self.X_MAIN_PANEL + (self.W_MAIN_PANEL / 2) - (self.W_EXIT / 2)
		self.Y_EXIT = self.Y_MAIN_PANEL + self.H_MAIN_PANEL - self.H_EXIT - 15
		
		self.X_PLANETS = self.X_MAIN_PANEL + 70
		self.Y_PLANETS_BEGIN = self.Y_MAIN_PANEL + 55
		self.H_PLANETS = 60
		
	def interfaceScreen(self, argsList):
		
#		FinalFrontier = CvEventInterface.getEventManager().FinalFrontier #FFPBUG
		
		self.EXIT_TEXT = localText.getText("TXT_KEY_SCREEN_CONTINUE", ())
		
		# Create screen
		
		screen = CyGInterfaceScreen( "CvPlanetInfoScreen", CvScreenEnums.PLANET_INFO_SCREEN )
		
		screen.showScreen(PopupStates.POPUPSTATE_QUEUED, False)
		screen.showWindowBackground( False )
		screen.setDimensions(screen.centerX(self.X_SCREEN), screen.centerY(self.Y_SCREEN), self.W_SCREEN, self.H_SCREEN)
		screen.enableWorldSounds( false )
		
		# Main
		szMainPanel = "DawnOfManMainPanel"
		screen.addPanel( szMainPanel, "", "", true, true,
			self.X_MAIN_PANEL, self.Y_MAIN_PANEL, self.W_MAIN_PANEL, self.H_MAIN_PANEL, PanelStyles.PANEL_STYLE_MAIN )
		
		# Planet Orbit Level Panels
		screen.addPanel( "Level1Panel", "", "", true, true,
			self.X_LEVEL_1_PANEL, self.Y_LEVEL_1_PANEL, self.W_LEVEL_1_PANEL, self.H_LEVEL_1_PANEL, PanelStyles.PANEL_STYLE_DAWNBOTTOM )
		screen.addPanel( "Level2Panel", "", "", true, true,
			self.X_LEVEL_2_PANEL, self.Y_LEVEL_2_PANEL, self.W_LEVEL_2_PANEL, self.H_LEVEL_2_PANEL, PanelStyles.PANEL_STYLE_DAWNBOTTOM )
		screen.addPanel( "Level3Panel", "", "", true, true,
			self.X_LEVEL_3_PANEL, self.Y_LEVEL_3_PANEL, self.W_LEVEL_3_PANEL, self.H_LEVEL_3_PANEL, PanelStyles.PANEL_STYLE_DAWNBOTTOM )
		
		# Planet Orbit Level Labels
		
		szBuffer = u"<font=3>" + localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_0", ()).upper() + u"</font>"
		screen.setText( "InfluenceLevel0Label", "Background", szBuffer, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_1_PANEL + (self.W_LEVEL_1_PANEL / 2), self.Y_LEVEL_1_PANEL + 15, -0.1, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		szBuffer = u"<font=3>" + localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_1", ()).upper() + u"</font>"
		screen.setText( "InfluenceLevel1Label", "Background", szBuffer, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_2_PANEL + (self.W_LEVEL_2_PANEL / 2), self.Y_LEVEL_2_PANEL + 15, -0.1, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		szBuffer = u"<font=3>" + localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_2", ()).upper() + u"</font>"
		screen.setText( "InfluenceLevel2Label", "Background", szBuffer, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_3_PANEL + (self.W_LEVEL_3_PANEL / 2), self.Y_LEVEL_3_PANEL + 15, -0.1, FontTypes.GAME_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		
#		szText = localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_0", ())
#		screen.setLabel( "InfluenceLevel0Label", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_1_PANEL + (self.W_LEVEL_1_PANEL / 2), self.Y_LEVEL_1_PANEL + 19, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
#		szText = localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_1", ())
#		screen.setLabel( "InfluenceLevel1Label", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_2_PANEL + (self.W_LEVEL_2_PANEL / 2), self.Y_LEVEL_2_PANEL + 19, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
#		szText = localText.getText("TXT_KEY_FF_INTERFACE_INFLUENCE_2", ())
#		screen.setLabel( "InfluenceLevel2Label", "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, self.X_LEVEL_3_PANEL + (self.W_LEVEL_3_PANEL / 2), self.Y_LEVEL_3_PANEL + 19, -0.1, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		
		screen.setButtonGFC("Exit", self.EXIT_TEXT, "", self.X_EXIT, self.Y_EXIT, self.W_EXIT, self.H_EXIT, WidgetTypes.WIDGET_CLOSE_SCREEN, -1, -1, ButtonStyles.BUTTON_STYLE_STANDARD )
		
		# Real Planet Stuff
		
		iX, iY= argsList
		
		pSystemID = CyMap().getSolarSystemID(iX, iY) #FFPBUG
		CySolarSystem = CyMap().getSolarSystem(pSystemID)
		
		# Loop through all (potential) planets
		for iPlanetLoop in range(1,9):
			
			iX = self.X_PLANETS
			iY = self.Y_PLANETS_BEGIN + ((self.H_PLANETS + 10) * (iPlanetLoop - 1))
			
			# Modify for Title Label space (for each Influence Level)
			#Final Frontier Plus change: count the 6th planet as part of ring three.
			if (iPlanetLoop > 3):
				iY += 50
			if (iPlanetLoop > 5):		#(iPlanetLoop > 6)
				iY += 50
			
			#pLoopPlanet = pSystem.getPlanet(iPlanetLoop)
			pLoopPlanet = CySolarSystem.getPlanetIDByRing( iPlanetLoop - 1)

			szGraphicName = "PlanetGraphic" + str(iPlanetLoop)
			szLabelName = "PlanetPop" + str(iPlanetLoop)
			
			if (pLoopPlanet != -1):
				
				# Planet widget
				CyPlanet=CySolarSystem.getPlanet(pLoopPlanet)

				sizeTag = aszPlanetSizes[CyPlanet.getSize()]
				filename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, sizeTag)
				screen.addModelGraphicGFC(szGraphicName, filename, iX, iY, self.H_PLANETS, self.H_PLANETS, WidgetTypes.WIDGET_GENERAL, -1, -1, -20, 30, 1.0)
				textureTag = aszPlanetTypes[CyPlanet.getType()]
				textureFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, textureTag)
				screen.changeModelGraphicTextureGFC(szGraphicName, textureFilename)
				
				self.addPlanetCustomization(CySolarSystem, CyPlanet, szGraphicName)
				
				screen.setModelGraphicRotationRateGFC(szGraphicName, -0.5)
				
				# Name Labels

				szText = CyPlanet.getName()
				if CyPlanet.isBonus(): # Planetary Resource Indicator
					szText += u" %c" % gc.getBonusInfo(CyPlanet.getBonus()).getChar() # Planetary Resource Indicator
				screen.setLabel( "PlanetNameLabel" + str(iPlanetLoop), "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, iX + self.H_PLANETS + 70, iY, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				
				# Yield Labels
				iFood = CyPlanet.getYield(CyGame().getActivePlayer(), 0)
				iProduction = CyPlanet.getYield(CyGame().getActivePlayer(), 1)
				iCommerce = CyPlanet.getYield(CyGame().getActivePlayer(), 2)
				
				szText = localText.getText("TXT_KEY_FF_INTERFACE_PLANET_SELECTION_HELP_0", (iFood, iProduction, iCommerce))
				screen.setLabel( "PlanetYieldLabel" + str(iPlanetLoop), "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, iX + self.H_PLANETS + 70, iY + 20, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
				
				# Population Labels
				szText = localText.getText("TXT_KEY_FF_INTERFACE_PLANET_POPULATION", (aiDefaultPopulationPlanetSizeTypes[CyPlanet.getSize()],))
				screen.setLabel( "PlanetPopulationLabel" + str(iPlanetLoop), "Background", szText, CvUtil.FONT_CENTER_JUSTIFY, iX + self.H_PLANETS + 70, iY + 40, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		
	def addPlanetCustomization(self, CySolarSystem, CyPlanet, szWidgetName, bShowSelection = true):
		
		screen = CyGInterfaceScreen( "CvPlanetInfoScreen", CvScreenEnums.PLANET_INFO_SCREEN )
		
		# Disabled by Doomsday Missile?
		if (CyPlanet.isDisabled()):
			szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, "FEATURE_MODEL_TAG_SUN_ORANGE")
			screen.addToModelGraphicGFC(szWidgetName, szFilename)
		
		else:
			
			iType = CyPlanet.getType()
			# Atmospheric effect
			if (iType == iPlanetTypeGreen or iType == iPlanetTypeOrange or iType == iPlanetTypeWhite):
				szPlanetArt = aszPlanetGlowSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Clouds effect
			if (iType == iPlanetTypeGreen):
				szPlanetArt = aszPlanetCloudsSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Rings effect
			if (CyPlanet.isRings()):
				szPlanetArt = aszPlanetRingsSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Moon
			if (CyPlanet.isMoon()):
				szPlanetArt = aszPlanetMoonSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Population Lights effect
			iPop = CyPlanet.getPopulation()
			if (iPop == 1):
				szPlanetArt = aszPlanetPopulation1Sizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			elif (iPop == 2):
				szPlanetArt = aszPlanetPopulation2Sizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			elif (iPop >= 3):
				szPlanetArt = aszPlanetPopulation3Sizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Mag-Lev Network
			iMagLevNetwork = CvUtil.findInfoTypeNum(gc.getBuildingInfo,gc.getNumBuildingInfos(),'BUILDING_MAG_LEV_NETWORK')
			if (CyPlanet.isBuilding(iMagLevNetwork)):
				szPlanetArt = aszPlanetMagLevNetworkSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
			
			# Commercial Satellites or PBS (UB for Brotherhood)
			iCommercialSatellites = CvUtil.findInfoTypeNum(gc.getBuildingInfo,gc.getNumBuildingInfos(),'BUILDING_COMMERCIAL_SATELLITES')
			iPBS = CvUtil.findInfoTypeNum(gc.getBuildingInfo,gc.getNumBuildingInfos(),'BUILDING_PBS')
			if (CyPlanet.isBuilding(iCommercialSatellites) or CyPlanet.isBuilding(iPBS)):
				szPlanetArt = aszPlanetCommercialSatellitesSizes[CyPlanet.getSize()]
				szFilename = ArtFileMgr.getFeatureArtInfo("ART_DEF_FEATURE_SOLAR_SYSTEM").getFeatureDummyNodeName(0, szPlanetArt)
				screen.addToModelGraphicGFC(szWidgetName, szFilename)
				
	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		return
		
	def update(self, fDelta):
		return
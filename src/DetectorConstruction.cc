//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// Authors: Susanna Guatelli and Francesco Romano
// susanna@uow.edu.au, francesco.romano@ct.infn.it

// Modified by Jacopo Magini: j.magini@surrey.ac.uk

#include "DetectorConstruction.hh"
#include "globals.hh"
#include "G4Element.hh"
#include "G4Material.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4LogicalVolume.hh"
#include "G4CSGSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
//#include "G4SubtractionSolid.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"
#include "SensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4UserLimits.hh"
#include "Randomize.hh"
#include "G4ThreeVector.hh"
#include "G4GeometryTolerance.hh"
#include "G4GeometryManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4NistManager.hh"

DetectorConstruction::DetectorConstruction(AnalysisManager* analysis_manager, DetectorMessenger* detector_messenger)
{
	analysis = analysis_manager;
	messenger = detector_messenger;
	
	/* the following are currently only implemented for MicroDiamond
	detectorType = messenger -> GetTheDetector(); // ignored in this fork
	detectorPositionDepth = messenger -> GetDetectorPositionDepth();
	detectorSizeWidth = messenger -> GetDetectorSizeWidth();
	detectorSizeThickness = messenger -> GetDetectorSizeThickness();
	secondStageSizeDim = messenger -> GetSecondStageSizeDim();
	secondStageThickness = messenger -> GetSecondStageThickness();
	usingPhantom = messenger -> GetUsingPhantomBool();
	multiSV = messenger -> GetMultiSVBool();
		
	if( multiSV == true )	nOfSV = 4;
	else if( multiSV == false )	nOfSV = 1;
	*/
}

DetectorConstruction::~DetectorConstruction(){

}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
	//Define Vacuum
	G4double A = 1.01*g/mole;
	G4double Z = 1.;
	G4double vacuumDensity = 1.e-25 *g/cm3;
	G4double pressure = 3.e-18*pascal;
	G4double temperature = 2.73*kelvin;
	G4Material* vacuum = new G4Material("Galactic", Z, A,
			         vacuumDensity,kStateGas,temperature,pressure);

	/*
	//Define Hydrogen 
	A = 1.01 * g/mole;
	Z = 1;
	G4Element* elH = new G4Element ("Hydrogen", "H", Z, A);
	
	//Define Oxygen
	A = 16.0 * g/mole;
	Z = 8;
	G4Element* elO = new G4Element ("Oxygen", "O", Z, A);

	//define water
	G4Material* water = new G4Material("water", 1*g/cm3, 2);
	water -> AddElement(elH, 2);
	water -> AddElement(elO, 1);
	*/
	
	G4Material* water = G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
	
	//Define volumes
	// World volume  has size 1cm
	G4double worldx = 0.5 * m;
	G4double worldy = 0.5 * m;
	G4double worldz = 1 * m;

	// World volume, containing all geometry
	G4Box* world = new G4Box("world_box", worldx, worldy, worldz);

	G4LogicalVolume* logical_world = new G4LogicalVolume(world, vacuum, "world_log", 0,0,0);

	//set the logical world volume invisible
	logical_world -> SetVisAttributes(G4VisAttributes::GetInvisible());

	G4VPhysicalVolume* physical_world = new G4PVPlacement(0,
								G4ThreeVector(),
								logical_world, 
								"world_phys",
								0, 
								false, 
								0);
	
	//water phantom
	//slices
	nOfSlices = 400 ;
	G4double sliceWidth = 1.*mm;
	
	G4double phantom_x = 200.*mm;
	G4double phantom_y = 200.*mm;
	G4double phantom_z = nOfSlices * sliceWidth ;
	
	G4Box* phantom_box = new G4Box("phantom_box", phantom_x/2, phantom_y/2, phantom_z/2);
	
	G4LogicalVolume* logical_phantom = new G4LogicalVolume(phantom_box, water, "phantom_log", 0,0,0);
	
	G4ThreeVector phantom_position = G4ThreeVector( 0., 0., phantom_z );	//the phantom starts at z=0
	
	 new G4PVPlacement(0, phantom_position, logical_phantom,"phantom_phys",
				logical_world, 
				false, 0, true);
	
	logical_phantom -> SetVisAttributes(G4VisAttributes(G4Colour(0., 0.2, 0.6)));

	G4Box* slice_box = new G4Box("slice_box", phantom_x/2, phantom_y/2, sliceWidth/2);
 
	G4LogicalVolume* logical_slice = new G4LogicalVolume(slice_box, water, "slice_log",0,0,0);
 
	logical_slice -> SetVisAttributes(G4VisAttributes(G4Colour(0., 0.2, 0.6)));
	
	new G4PVReplica(
                 "Slice",          // its name
                 logical_slice,          // its logical volume
                 logical_phantom,          // its mother
                 kZAxis,           // axis of replication
                 nOfSlices,        // number of replica
                 sliceWidth);  // witdth of replica

	return physical_world;
}

void DetectorConstruction::ConstructSDandField()
{
   SensitiveDetector* SD = new SensitiveDetector("SD", "DetectorHitsCollection", analysis, nOfSlices);
   G4SDManager::GetSDMpointer()->AddNewDetector(SD);
   SetSensitiveDetector("slice_log", SD);


}

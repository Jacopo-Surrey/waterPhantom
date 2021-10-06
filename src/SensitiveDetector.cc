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
// Authors: Susanna Guatelli, susanna@uow.edu.au,
// Authors: Jeremy Davis, jad028@uowmail.edu.au
//
//  code based on the basic example B2
//
#include "SensitiveDetector.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

#include "DetectorConstruction.hh"

#include "G4RunManager.hh"

SensitiveDetector::SensitiveDetector(const G4String& name,
						const G4String& hitsCollectionName,
						AnalysisManager* analysis_manager,
						G4int nOfSlices
							) 
	: G4VSensitiveDetector(name),
	fHitsCollection(NULL)
{
	collectionName.insert(hitsCollectionName);
	analysis = analysis_manager;
	nOfLayers = nOfSlices;
	
	// retrieve the name of the active volume
	//std::ostringstream AVname;
	//AVname << "SV_phys_" << DetectorConstruction::getActiveSVno();
	//activeVolumeName = AVname.str();
	
	/*activeVolumeName = "SV_phys_1";
	
	// initialize the private variables for primaries energy lost calculation
	firstStep=true;
	Ek_in=0.;
	Ek_out=0.;*/
}

SensitiveDetector::~SensitiveDetector() 
{}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce)
{
	// Create hits collection
	fHitsCollection 
		= new SensitiveDetectorHitsCollection(SensitiveDetectorName, collectionName[0]); 

	// Add this collection in hce
	G4int hcID 
		= G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
	hce->AddHitsCollection( hcID, fHitsCollection ); 
	
	// Prepare hits for various layers
	//for (G4int i=0; i<nOfLayers; i++ ) {
	//	fHitsCollection -> insert( new SensitiveDetectorHit() );
	//}
}


G4bool SensitiveDetector::ProcessHits(G4Step* aStep, 
                                     G4TouchableHistory*)
{  
	// energy deposit
	G4double edep = aStep->GetTotalEnergyDeposit();

	if (edep==0.) return false;

	//G4String volumeName = aStep -> GetPreStepPoint() -> GetPhysicalVolume()-> GetName();

	//if(volumeName != activeVolumeName) 
	//	return false;  

	SensitiveDetectorHit* thisHit = new SensitiveDetectorHit();
	
	G4int layerNumber = aStep ->
							GetPreStepPoint() ->
							GetTouchable() ->
							GetReplicaNumber() ;
							
	/*SensitiveDetectorHit* thisHit = (*fHitsCollection)[layerNumber];
	
	 if ( ! thisHit ) {
		G4ExceptionDescription msg;
		msg << "Cannot access hit " << layerNumber;
		G4Exception("SensitiveDetector::ProcessHits()",
			"MyCode0004", FatalException, msg);
	}*/
	
	thisHit -> SetLayerID(layerNumber);

	thisHit -> SetEdep(edep);
	
	// step length
	G4double len = aStep->GetStepLength();
	
	const G4ParticleDefinition* thisParticle = aStep ->
											GetTrack() ->
											GetParticleDefinition() ;
	
	// only consider ions, protons, and neutrons for path length
	G4String particleName = thisParticle -> GetParticleName();
								
	G4int particleZ = thisParticle -> GetAtomicNumber();
	
	thisHit -> SetZZ( particleZ );
	
	if ( (particleZ < 1) && (particleName != "neutron"))
		len = 0.;

	thisHit -> SetPath(len);
	
	/*
	// Primary kinetic energy storing
	G4int trackID = aStep -> GetTrack() -> GetTrackID();
	if (trackID==1) { //if it is a primary particle
		// initial kinetic energy, when entering the active volume
		if (firstStep==1) {
			Ek_in=aStep->GetPreStepPoint()->GetKineticEnergy();
			firstStep=false;
			// print out the position where the primary gets into the active volume. It is a check so it should be comment out when running the simulation.
			//G4ThreeVector posEntrance=aStep->GetPreStepPoint()->GetPosition();
			//G4cout << "Primary entrance position " <<posEntrance << G4endl;
		}
		Ek_out=aStep->GetPostStepPoint()->GetKineticEnergy(); // it updates it every step until the last one, which is the one I am interested in storing
	}*/
	
	fHitsCollection->insert(thisHit);
	return true;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent*)
{
	// Initialisation of total energy deposition per event to zero
	/*G4double totalEdepInOneEvent=0;
	G4double totalPathLengthInOneEvent=0;
	G4int maxZinsideDetector=0;
	*/
	
	G4double* totalEdepInOneEvent = new G4double[nOfLayers];
	G4double* totalPathLengthInOneEvent = new G4double[nOfLayers];
	G4int* maxZinsideDetector = new G4int[nOfLayers];
	
	for (G4int i=0; i<nOfLayers; i++ ) {
		totalEdepInOneEvent[i]=0;
		totalPathLengthInOneEvent[i]=0;
		maxZinsideDetector[i]=0;
	}
	
	G4int eventID = G4RunManager::GetRunManager() ->
						GetCurrentEvent() ->
						GetEventID();
 
	G4int NbHits = fHitsCollection->entries();
   //G4cout << "number of hits " <<NbHits << G4endl;
   
	G4int layer; G4double edep; G4double len; G4int zz;
	
	for (G4int i=0;i<NbHits;i++) 
	{
		layer = (*fHitsCollection)[i]->GetLayerID();
		edep = (*fHitsCollection)[i]->GetEdep();
		len = (*fHitsCollection)[i]->GetPath();
		zz = (*fHitsCollection)[i]->GetZZ(); 

		totalEdepInOneEvent[layer] = totalEdepInOneEvent[layer] + edep;
		totalPathLengthInOneEvent[layer] = totalPathLengthInOneEvent[layer] + len;
		
		maxZinsideDetector[layer] = std::max( maxZinsideDetector[layer] , zz );
	} 

	for (G4int i=0; i<nOfLayers; i++ ) {
		if (totalEdepInOneEvent[i]!=0)
			analysis-> StoreEnergyDeposition(
						totalEdepInOneEvent[i],
						totalPathLengthInOneEvent[i],
						maxZinsideDetector[i],
						i,
						eventID
				);
	}

	/*G4double elost = Ek_in - Ek_out;
	if (elost>0)
		analysis-> StorePrimaryEnergyLost(elost, Ek_in, Ek_out);
	// restore private variables to default values
	firstStep=true;
	Ek_in=0.;
	Ek_out=0.;*/
	
	delete totalEdepInOneEvent;
	delete totalPathLengthInOneEvent;
	delete maxZinsideDetector;
}

/*-----------------------------------------------------------------------*\
|                                                                         |
|                    ╔═══╦═╗╔═╦═══╦╗╔═╦═══╗                               |
|                    ║╔═╗║║╚╝║║╔═╗║║║╔╣╔══╝                               | 
|   ╔╗╔══╦╗╔╦╦═╗╔══╦═╣╚══╣╔╗╔╗║║ ║║╚╝╝║╚══╗                               |
|   ║║║╔╗║╚╝╠╣╔╗╣╔╗║╔╩══╗║║║║║║║ ║║╔╗║║╔══╝                               |
|   ║╚╣╔╗║║║║║║║║╔╗║║║╚═╝║║║║║║╚═╝║║║╚╣╚══╗                               |
|   ╚═╩╝╚╩╩╩╩╩╝╚╩╝╚╩╝╚═══╩╝╚╝╚╩═══╩╝╚═╩═══╝                               |
|                                                                         |
|                                                                         |
|   Authors: A. Cuoci                                                     |
|                                                                         |
|   Contacts: Alberto Cuoci                                               |
|   email: alberto.cuoci@polimi.it                                        |
|   Department of Chemistry, Materials and Chemical Engineering           |
|   Politecnico di Milano                                                 |
|   P.zza Leonardo da Vinci 32, 20133 Milano (Italy)                      |
|                                                                         |
|-------------------------------------------------------------------------|
|                                                                         |
|   This file is part of laminarSMOKE solver.                             |
|                                                                         |
|   License                                                               |
|                                                                         |
|   Copyright(C) 2016, 2015, 2014 A. Cuoci                                |
|   laminarSMOKE is free software: you can redistribute it and/or modify  |
|   it under the terms of the GNU General Public License as published by  |
|   the Free Software Foundation, either version 3 of the License, or     |
|   (at your option) any later version.                                   |
|                                                                         |
|   laminarSMOKE is distributed in the hope that it will be useful,       |
|   but WITHOUT ANY WARRANTY; without even the implied warranty of        |
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
|   GNU General Public License for more details.                          |
|                                                                         |
|   You should have received a copy of the GNU General Public License     |
|   along with laminarSMOKE. If not, see <http://www.gnu.org/licenses/>.  |
|                                                                         |
|                                                                         |
|   Application: laminarBuoyantSimpleSMOKE                                |
|                                                                         |
|   Description: steady solver for reactive flows with detailed kinetic   |
|                mechanisms.                                              |
|                                                                         |
\*-----------------------------------------------------------------------*/

// This is a steady state simulation
#define STEADYSTATE 1

// OpenSMOKE++ Definitions
#include "OpenSMOKEpp"

// CHEMKIN maps
#include "maps/Maps_CHEMKIN"
#include "utilities/soot/polimi/OpenSMOKE_PolimiSoot_Analyzer.h"

// OpenSMOKE++ Dictionaries
#include "dictionary/OpenSMOKE_Dictionary"

// OpenFOAM
#include "fvCFD.H"
#include "multivariateScheme.H"
#include "simpleControl.H"
#if OPENFOAM_VERSION >= 40
#include "fvOptions.H"
#else
#include "fvIOoptionList.H"
#endif
#include "interpolation.H"
#include "fixedFluxPressureFvPatchScalarField.H"

// Customized radiation model
#include "OpenSMOKEradiationModel.H"

// Additional include files
#include "sparkModel.H"
#include "utilities.H"
#include "laminarSMOKEthermoClass.H"

// Virtual chemistry
#include "utilities/virtualchemistry/VirtualChemistry.h"

// Linearization
#include "linearModel.H"

// Soot
#include "sootUtilities.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
	#include "setRootCase.H"
	#include "createTime.H"
	#include "createMesh.H"
	#include "readGravitationalAcceleration.H"

	#if OPENFOAM_VERSION >= 40
	#include "createControl.H"
	#else
	simpleControl simple(mesh);
	#endif

	#include "createBasicFields.H"
	#include "readOptions.H"
	#include "readOptionsSteadyState.H"

	linearModel linear_model(*thermodynamicsMapXML, *kineticsMapXML);
	
	#include "createGasFields.H"
	#include "createChemicalFields.H"
	#include "createLaminarSMOKEThermoClass.H"
        #if OPENFOAM_VERSION >= 30
        #include "createMRF.H"
        #endif
	#include "createFvOptions.H"
	#include "createRadiationModel.H"
	#include "memoryAllocation.H"
	#include "properties.H"
	#include "createBuoyantAdditionalFields.H"
	#include "initContinuityErrs.H"

	dimensionedScalar initialMass = fvc::domainIntegrate(rho);

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (simple.loop())
    {
         Info<< "Time = " << runTime.timeName() << nl << endl;
		
	 double t0 = runTime.value() - runTime.deltaT().value();
	 double tf = runTime.value();

	 // Pressure-velocity SIMPLE corrector
         {
		if (momentumEquations == true)
		{
		    #include "UEqn.H"

		    #include "updateProperties.H"
		    #include "jacobianEvaluation.H"

		    #include "fluxes.H"
		    #include "YEqn.H"
		    #include "TEqn.H" 

		    // Pressure equation
		    #include "pEqn.H"
		}
		else
		{
		    #include "updateProperties.H"
		    #include "jacobianEvaluation.H"

		    #include "fluxes.H"
		    #include "YEqn.H"
		    #include "TEqn.H" 
		}
	    
		// Passive scalars
	        #include "zMixEqn.H"
		#include "tauEqn.H"
         }	

	 #include "localPostProcessing.H"
				
	 runTime.write();
		
         Info << "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
              << "  ClockTime = " << runTime.elapsedClockTime() << " s"
              << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //

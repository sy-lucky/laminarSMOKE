/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2015 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "P1.H"
#include "fvmLaplacian.H"
#include "fvmSup.H"
#include "OpenSMOKEabsorptionEmissionModel.H"
#include "OpenSMOKEscatterModel.H"
#include "constants.H"
#include "addToRunTimeSelectionTable.H"

using namespace Foam::constant;

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    namespace radiation
    {
        defineTypeNameAndDebug(P1, 0);
        addToRadiationRunTimeSelectionTables(P1);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::radiation::P1::P1(const volScalarField& T)
:
    OpenSMOKEradiationModel(typeName, T),
    G_
    (
        IOobject
        (
            "G",
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_
    ),
    Qr_
    (
        IOobject
        (
            "Qr",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("Qr", dimMass/pow3(dimTime), 0.0)
    ),
    a_
    (
        IOobject
        (
            "a",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("a", dimless/dimLength, 0.0)
    ),
    e_
    (
        IOobject
        (
            "e",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("a", dimless/dimLength, 0.0)
    ),
    E_
    (
        IOobject
        (
            "E",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("E", dimMass/dimLength/pow3(dimTime), 0.0)
    )
{}


Foam::radiation::P1::P1(const dictionary& dict, const volScalarField& T)
:
    OpenSMOKEradiationModel(typeName, dict, T),
    G_
    (
        IOobject
        (
            "G",
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_
    ),
    Qr_
    (
        IOobject
        (
            "Qr",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("Qr", dimMass/pow3(dimTime), 0.0)
    ),
    a_
    (
        IOobject
        (
            "a",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("a", dimless/dimLength, 0.0)
    ),
    e_
    (
        IOobject
        (
            "e",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("a", dimless/dimLength, 0.0)
    ),
    E_
    (
        IOobject
        (
            "E",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("E", dimMass/dimLength/pow3(dimTime), 0.0)
    )
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::radiation::P1::~P1()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::radiation::P1::read()
{
    if (OpenSMOKEradiationModel::read())
    {
        // nothing to read

        return true;
    }
    else
    {
        return false;
    }
}


void Foam::radiation::P1::calculate()
{
    a_ = absorptionEmission_->a();
    e_ = absorptionEmission_->e();
    E_ = absorptionEmission_->E();
    const volScalarField sigmaEff(scatter_->sigmaEff());

    const dimensionedScalar a0 ("a0", a_.dimensions(), ROOTVSMALL);

    // Construct diffusion
    const volScalarField gamma
    (
        IOobject
        (
            "gammaRad",
            G_.mesh().time().timeName(),
            G_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        1.0/(3.0*a_ + sigmaEff + a0)
    );

    // Solve G transport equation
    solve
    (
        fvm::laplacian(gamma, G_)
      - fvm::Sp(a_, G_)
     ==
      - 4.0*(e_*physicoChemical::sigma*pow4(T_) ) - E_
    );

    // Calculate radiative heat flux on boundaries.
    forAll(mesh_.boundaryMesh(), patchi)
    {
        if (!G_.boundaryField()[patchi].coupled())
        {
		#if OPENFOAM_VERSION >= 40
    		Qr_.boundaryFieldRef()[patchi] = -gamma.boundaryField()[patchi]*G_.boundaryField()[patchi].snGrad();
    		#else
   	 	Qr_.boundaryField()[patchi] = -gamma.boundaryField()[patchi]*G_.boundaryField()[patchi].snGrad();
    		#endif   
        }
    }
}


Foam::tmp<Foam::volScalarField> Foam::radiation::P1::Rp() const
{
    return tmp<volScalarField>
    (
        new volScalarField
        (
            IOobject
            (
                "Rp",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            4.0*absorptionEmission_->eCont()*physicoChemical::sigma
        )
    );
}


Foam::tmp<Foam::DimensionedField<Foam::scalar, Foam::volMesh> >
Foam::radiation::P1::Ru() const
{

    #if OPENFOAM_VERSION >= 40
    	const DimensionedField<scalar, volMesh>& G = G_();
	    const DimensionedField<scalar, volMesh>  E = absorptionEmission_->ECont()()();
	    const DimensionedField<scalar, volMesh>  a = absorptionEmission_->aCont()()();
    #else
   	 const DimensionedField<scalar, volMesh>& G = G_.dimensionedInternalField();
   	 const DimensionedField<scalar, volMesh>  E = absorptionEmission_->ECont()().dimensionedInternalField();
   	 const DimensionedField<scalar, volMesh>  a = absorptionEmission_->aCont()().dimensionedInternalField();
    #endif



    return a*G - E;
}


// ************************************************************************* //

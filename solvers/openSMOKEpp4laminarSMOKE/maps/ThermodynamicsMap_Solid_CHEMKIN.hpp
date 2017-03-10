/*----------------------------------------------------------------------*\
|    ___                   ____  __  __  ___  _  _______                  |
|   / _ \ _ __   ___ _ __ / ___||  \/  |/ _ \| |/ / ____| _     _         |
|  | | | | '_ \ / _ \ '_ \\___ \| |\/| | | | | ' /|  _| _| |_ _| |_       |
|  | |_| | |_) |  __/ | | |___) | |  | | |_| | . \| |__|_   _|_   _|      |
|   \___/| .__/ \___|_| |_|____/|_|  |_|\___/|_|\_\_____||_|   |_|        |
|        |_|                                                              |
|                                                                         |
|   Author: Alberto Cuoci <alberto.cuoci@polimi.it>                       |
|   CRECK Modeling Group <http://creckmodeling.chem.polimi.it>            |
|   Department of Chemistry, Materials and Chemical Engineering           |
|   Politecnico di Milano                                                 |
|   P.zza Leonardo da Vinci 32, 20133 Milano                              |
|                                                                         |
|-------------------------------------------------------------------------|
|                                                                         |
|   This file is part of OpenSMOKE++ framework.                           |
|                                                                         |
|	License                                                               |
|                                                                         |
|   Copyright(C) 2014, 2013, 2012  Alberto Cuoci                          |
|   OpenSMOKE++ is free software: you can redistribute it and/or modify   |
|   it under the terms of the GNU General Public License as published by  |
|   the Free Software Foundation, either version 3 of the License, or     |
|   (at your option) any later version.                                   |
|                                                                         |
|   OpenSMOKE++ is distributed in the hope that it will be useful,        |
|   but WITHOUT ANY WARRANTY; without even the implied warranty of        |
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
|   GNU General Public License for more details.                          |
|                                                                         |
|   You should have received a copy of the GNU General Public License     |
|   along with OpenSMOKE++. If not, see <http://www.gnu.org/licenses/>.   |
|                                                                         |
\*-----------------------------------------------------------------------*/

#include "math/OpenSMOKEUtilities.h"

namespace OpenSMOKE
{ 
	ThermodynamicsMap_Solid_CHEMKIN::ThermodynamicsMap_Solid_CHEMKIN(rapidxml::xml_document<>& doc)
	{
		ImportSpeciesFromXMLFile(doc);
		this->ImportElementsFromXMLFile(doc);
		MemoryAllocation();
		ImportCoefficientsFromXMLFile(doc);
	}

	void ThermodynamicsMap_Solid_CHEMKIN::MemoryAllocation()
	{
		Cp_LT = new double[5*this->nspecies_];	
		Cp_HT = new double[5*this->nspecies_];
		DH_LT = new double[6*this->nspecies_];
		DH_HT = new double[6*this->nspecies_];
		DS_LT = new double[6*this->nspecies_];
		DS_HT = new double[6*this->nspecies_];
		TL = new double[this->nspecies_];	
		TH = new double[this->nspecies_];
		TM = new double[this->nspecies_];

		ChangeDimensions(this->nspecies_, &this->MW_, true);
		ChangeDimensions(this->nspecies_, &this->uMW_, true);

		ChangeDimensions(this->nspecies_, &species_cp_over_R_, true);
		ChangeDimensions(this->nspecies_, &species_h_over_RT_, true);
		ChangeDimensions(this->nspecies_, &species_g_over_RT_, true);
		ChangeDimensions(this->nspecies_, &species_s_over_R_, true);
                
		ChangeDimensions(this->nspecies_, &aux_vector_total_number_species_, true);

		cp_must_be_recalculated_ = true;
		h_must_be_recalculated_ = true;
		s_must_be_recalculated_ = true;
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::SetTemperature(const double& T)
	{
		this->T_ = T;
		cp_must_be_recalculated_ = true;
		h_must_be_recalculated_ = true;
		s_must_be_recalculated_ = true;
	}

	void ThermodynamicsMap_Solid_CHEMKIN::SetPressure(const double& P)
	{
		this->P_ = P;
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::SetCoefficients(const unsigned k, const double* coefficients)
	{
		const double one_third = 1./3.;

		// Specific heat: high temperature
		{
			unsigned int i = k*5;
			Cp_HT[i++] = coefficients[0];
			Cp_HT[i++] = coefficients[1];
			Cp_HT[i++] = coefficients[2];
			Cp_HT[i++] = coefficients[3];
			Cp_HT[i++] = coefficients[4];
		}

		// Specific heat: low temperature
		{
			unsigned int i = k*5;
			Cp_LT[i++] = coefficients[7];
			Cp_LT[i++] = coefficients[8];
			Cp_LT[i++] = coefficients[9];
			Cp_LT[i++] = coefficients[10];
			Cp_LT[i++] = coefficients[11];
		}

		// Enthalpy: high temperature
		{
			unsigned int j = k*5;
			unsigned int i = k*6;
			DH_HT[i++] = Cp_HT[j++];
			DH_HT[i++] = 0.50 *Cp_HT[j++];
			DH_HT[i++] = one_third*Cp_HT[j++];
			DH_HT[i++] = 0.25 *Cp_HT[j++];
			DH_HT[i++] = 0.20 *Cp_HT[j++];
			DH_HT[i++] = coefficients[5];
		}
	
		// Enthalpy: low temperature
		{
			unsigned int j = k*5;
			unsigned int i = k*6;
			DH_LT[i++] = Cp_LT[j++];
			DH_LT[i++] = 0.50 *Cp_LT[j++];
			DH_LT[i++] = one_third*Cp_LT[j++];
			DH_LT[i++] = 0.25 *Cp_LT[j++];
			DH_LT[i++] = 0.20 *Cp_LT[j++];
			DH_LT[i++] = coefficients[12];
		}

		// Entropy: high temperature
		{
			unsigned int j = k*5;
			unsigned int i = k*6;
			DS_HT[i++] = Cp_HT[j++];
			DS_HT[i++] = Cp_HT[j++];
			DS_HT[i++] = 0.50 *Cp_HT[j++];
			DS_HT[i++] = one_third*Cp_HT[j++];
			DS_HT[i++] = 0.25 *Cp_HT[j++];
			DS_HT[i++] = coefficients[6];
		}

		// Entropy: low temperature
		{
			unsigned int j = k*5;
			unsigned int i = k*6;
			DS_LT[i++] = Cp_LT[j++];
			DS_LT[i++] = Cp_LT[j++];
			DS_LT[i++] = 0.50 *Cp_LT[j++];
			DS_LT[i++] = one_third*Cp_LT[j++];
			DS_LT[i++] = 0.25 *Cp_LT[j++];
			DS_LT[i++] = coefficients[13];
		}

		// Temperature limits
		{
			TL[k] = coefficients[14];
			TH[k] = coefficients[15];
			TM[k] = coefficients[16];
		}

		this->MW_[k+1] = coefficients[17];
		this->uMW_[k+1] = 1./coefficients[17];
	}

	void ThermodynamicsMap_Solid_CHEMKIN::ImportCoefficientsFromASCIIFile(std::ifstream& fInput)
	{
		std::cout << " * Reading thermodynamic coefficients of species..." << std::endl;
		double coefficients[18];
		for(unsigned int i=0;i<this->nspecies_;i++)
		{
			for(unsigned int j=0;j<18;j++)
				fInput >> coefficients[j];
			SetCoefficients(i, coefficients);
		}
	}

	void ThermodynamicsMap_Solid_CHEMKIN::ImportSpeciesFromXMLFile(rapidxml::xml_document<>& doc)
	{
		// Names of species
		{
			rapidxml::xml_node<>* opensmoke_node = doc.first_node("opensmoke");
			rapidxml::xml_node<>* number_of_species_node = opensmoke_node->first_node("NumberOfSpecies");
			rapidxml::xml_node<>* names_of_species_node = opensmoke_node->first_node("NamesOfSpecies");
			
			rapidxml::xml_node<>* number_of_materials_node = opensmoke_node->first_node("NumberOfMaterials");
			rapidxml::xml_node<>* number_of_solid_species_node = opensmoke_node->first_node("NumberOfSolidSpecies");

			rapidxml::xml_node<>* names_of_solid_species_node = opensmoke_node->first_node("SolidSpecies");

			try
			{
				this->nspecies_ = boost::lexical_cast<unsigned int>(boost::trim_copy(std::string(number_of_species_node->value())));
				this->names_.resize(this->nspecies_);
				std::stringstream names_of_species_string;
				names_of_species_string << names_of_species_node->value();
				for(unsigned int i=0;i<this->nspecies_;i++)
					names_of_species_string >> this->names_[i]; 	
				
				// Materials
				number_of_materials_    = boost::lexical_cast<unsigned int>(boost::trim_copy(std::string(number_of_materials_node->value())));
				number_of_solid_species_ = boost::lexical_cast<unsigned int>(boost::trim_copy(std::string(number_of_solid_species_node->value())));
				number_of_gas_species_  = this->nspecies_ - number_of_solid_species_;
				
				// Solid species
				{
					vector_names_solid_species_.resize(number_of_solid_species_);
					std::stringstream names_solid_species_string;
					names_solid_species_string << names_of_solid_species_node->value();
					for(unsigned int i=0;i<number_of_solid_species_;i++)
						names_solid_species_string >> vector_names_solid_species_[i]; 	
				}
			}
			catch(...)
			{
				ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::ImportSpeciesFromXMLFile", "Error in reading the list of species.");
			}
		}

		// Reading all the available materials
		{
			matrix_names_solid_species_.resize(number_of_materials_);
			matrix_indices_solid_species_.resize(number_of_materials_);

			rapidxml::xml_node<>* opensmoke_node = doc.first_node("opensmoke");

			unsigned k = 0;
			for (rapidxml::xml_node<> *current_node = opensmoke_node->first_node("MaterialDescription"); current_node; current_node = current_node->next_sibling("MaterialDescription"))
			{
				std::cout << "Reading material description... " << std::endl;

				rapidxml::xml_attribute<> *pMaterialName = current_node->first_attribute("name");
				rapidxml::xml_attribute<> *pMaterialIndex = current_node->first_attribute("index");
									
				unsigned int n;
				std::stringstream dummy;
				dummy << current_node->value();

				dummy >> n;
				matrix_names_solid_species_[k].resize(n);
				matrix_indices_solid_species_[k].resize(n);

				for(unsigned int i=0;i<n;i++)
				{
					dummy >> matrix_names_solid_species_[k][i];
					dummy >> matrix_indices_solid_species_[k][i];
				}

				k++;
			}
		}
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::ImportCoefficientsFromXMLFile(rapidxml::xml_document<>& doc)
	{
		std::cout << " * Reading thermodynamic coefficients of species from XML file..." << std::endl;

		rapidxml::xml_node<>* opensmoke_node = doc.first_node("opensmoke");

		rapidxml::xml_node<>* thermodynamics_node = opensmoke_node->first_node("Thermodynamics");
		if (thermodynamics_node != 0)
		{
			std::string thermodynamics_type = thermodynamics_node->first_attribute("type")->value();			
			if (thermodynamics_type == "NASA")
			{
				rapidxml::xml_node<>* nasa_coefficients_node = thermodynamics_node->first_node("NASA-coefficients");
				
				std::stringstream nasa_coefficients;
				nasa_coefficients << nasa_coefficients_node->value();
			
				double coefficients[18];
				for(unsigned int i=0;i<this->nspecies_;i++)
				{
					for(unsigned int j=0;j<18;j++)
						nasa_coefficients >> coefficients[j];
					SetCoefficients(i, coefficients);
				}
			}
			else
				ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::ImportCoefficientsFromXMLFile", "Thermodynamics type not supported!"); 
		}
		else
			ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::ImportCoefficientsFromXMLFile", "Thermodynamics tag was not found!");
	}
 
	inline void ThermodynamicsMap_Solid_CHEMKIN::MolecularWeight_From_MoleFractions(double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::MolecularWeight_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::MolecularWeight_From_MassFractions(double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& y)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::MolecularWeight_From_MassFractions", "Function not available for Solid thermodynamics!");
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::MassFractions_From_MoleFractions(OpenSMOKE::OpenSMOKEVectorDouble& y, double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::MassFractions_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}
	
	inline void ThermodynamicsMap_Solid_CHEMKIN::MoleFractions_From_MassFractions(OpenSMOKE::OpenSMOKEVectorDouble& x, double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& y)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::MoleFractions_From_MassFractions", "Function not available for Solid thermodynamics!");
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::SolidMolecularWeight_From_SolidMoleFractions(double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{	
        MW = 0.;
        for (unsigned int j=1;j<=number_of_solid_species_;j++)
            MW += x[j] * this->MW_[number_of_gas_species_ + j];
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::SolidMolecularWeight_From_SolidMassFractions(double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& y)
	{
        MW = 0.;
        for (unsigned int j=1;j<=number_of_solid_species_;j++)
            MW += y[j] * this->uMW_[number_of_gas_species_ + j];
        MW = 1./MW;
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::SolidMassFractions_From_SolidMoleFractions(OpenSMOKE::OpenSMOKEVectorDouble& y, double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
        MW = 0.;
        for (unsigned int j=1;j<=number_of_solid_species_;j++)
            MW += x[j] * this->MW_[number_of_gas_species_ + j];
        for (unsigned int j=1;j<=number_of_solid_species_;j++)
            y[j] = x[j] * this->MW_[number_of_gas_species_ + j] / MW;
    }
	
	inline void ThermodynamicsMap_Solid_CHEMKIN::SolidMoleFractions_From_SolidMassFractions(OpenSMOKE::OpenSMOKEVectorDouble& x, double& MW, const OpenSMOKE::OpenSMOKEVectorDouble& y)
	{
            MW = 0.;
            for (unsigned int j=1;j<=number_of_solid_species_;j++)
                MW += y[j] * this->uMW_[number_of_gas_species_ + j];
            MW = 1./MW;
            for (unsigned int j=1;j<=number_of_solid_species_;j++)
                x[j] = y[j] / this->MW_[number_of_gas_species_ + j] * MW;
        }
 
	inline void ThermodynamicsMap_Solid_CHEMKIN::cp_over_R()
	{
		if (cp_must_be_recalculated_ == true)
		{
			const double T2 = this->T_*this->T_;
			const double T3 = T2*this->T_;
			const double T4 = T3*this->T_;

			unsigned int j = 0;
			for (unsigned int k=1;k<=this->nspecies_;k++)
			{
				species_cp_over_R_[k] = (this->T_>TM[k-1]) ?		Cp_HT[j] + this->T_*Cp_HT[j+1] + T2*Cp_HT[j+2] + T3*Cp_HT[j+3] + T4*Cp_HT[j+4] :
															Cp_LT[j] + this->T_*Cp_LT[j+1] + T2*Cp_LT[j+2] + T3*Cp_LT[j+3] + T4*Cp_LT[j+4] ;
				j += 5;
			}

			cp_must_be_recalculated_ = false;
		}
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::h_over_RT()
	{
		if (h_must_be_recalculated_ == true)
		{
			const double T2 = this->T_*this->T_;
			const double T3 = T2*this->T_;
			const double T4 = T3*this->T_;
			const double uT = 1./this->T_;

			int j = 0;
			for (unsigned int k=1;k<=this->nspecies_;k++)
			{
				species_h_over_RT_[k] = (this->T_>TM[k-1]) ?	DH_HT[j] + this->T_*DH_HT[j+1] + T2*DH_HT[j+2] + T3*DH_HT[j+3] + T4*DH_HT[j+4] + uT*DH_HT[j+5]:
														DH_LT[j] + this->T_*DH_LT[j+1] + T2*DH_LT[j+2] + T3*DH_LT[j+3] + T4*DH_LT[j+4] + uT*DH_LT[j+5];
				j += 6;
			}

			h_must_be_recalculated_ = false;
		}
	}
	
	inline void ThermodynamicsMap_Solid_CHEMKIN::s_over_R()
	{
		if (s_must_be_recalculated_ == true)
		{
			const double logT = log(this->T_);
			const double T2 = this->T_*this->T_;
			const double T3 = T2*this->T_;
			const double T4 = T3*this->T_;

			unsigned int j = 0;
			for (unsigned int k=1;k<=this->nspecies_;k++)
			{
				species_s_over_R_[k] = (this->T_>TM[k-1]) ?	DS_HT[j]*logT + this->T_*DS_HT[j+1] + T2*DS_HT[j+2] + T3*DS_HT[j+3] + T4*DS_HT[j+4] + DS_HT[j+5] :
															DS_LT[j]*logT + this->T_*DS_LT[j+1] + T2*DS_LT[j+2] + T3*DS_LT[j+3] + T4*DS_LT[j+4] + DS_LT[j+5] ;
				j += 6;
			}

			s_must_be_recalculated_ = false;
		}
	}

	inline void ThermodynamicsMap_Solid_CHEMKIN::g_over_RT()
	{
		h_over_RT();
		s_over_R();

		Sub(species_h_over_RT_, species_s_over_R_, &species_g_over_RT_);
	}

	void ThermodynamicsMap_Solid_CHEMKIN::cpMolar_Mixture_From_MoleFractions(double& cpmix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::cpMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}
	 
	void ThermodynamicsMap_Solid_CHEMKIN::hMolar_Mixture_From_MoleFractions(double& hmix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::hMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}

	void ThermodynamicsMap_Solid_CHEMKIN::sMolar_Mixture_From_MoleFractions(double& smix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::sMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}

	void ThermodynamicsMap_Solid_CHEMKIN::uMolar_Mixture_From_MoleFractions(double& umix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::uMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::gMolar_Mixture_From_MoleFractions(double& gmix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::gMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::aMolar_Mixture_From_MoleFractions(double& amix, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::aMolar_Mixture_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}

	void ThermodynamicsMap_Solid_CHEMKIN::cpMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& cp_species)
	{
		cp_over_R();
		Product(PhysicalConstants::R_J_kmol, species_cp_over_R_, &cp_species);
	}

	void ThermodynamicsMap_Solid_CHEMKIN::cpMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& cp_solidspecies)
	{
                cpMolar_Species(aux_vector_total_number_species_);
                for (unsigned int j=1;j<=number_of_solid_species_;j++)
                    cp_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}
        
	void ThermodynamicsMap_Solid_CHEMKIN::hMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& h_species)
	{
		h_over_RT();
		Product(PhysicalConstants::R_J_kmol*this->T_, species_h_over_RT_, &h_species);
	}
        
	void ThermodynamicsMap_Solid_CHEMKIN::hMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& h_solidspecies)
	{
		hMolar_Species(aux_vector_total_number_species_);
                for (unsigned int j=1;j<=number_of_solid_species_;j++)
                    h_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}        

	void ThermodynamicsMap_Solid_CHEMKIN::sMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& s_species)
	{
		s_over_R();
		Product(PhysicalConstants::R_J_kmol, species_s_over_R_, &s_species);
	}
        
	void ThermodynamicsMap_Solid_CHEMKIN::sMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& s_solidspecies)
	{
		sMolar_Species(aux_vector_total_number_species_);
                for (unsigned int j=1;j<=number_of_solid_species_;j++)
                    s_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}
         
	void ThermodynamicsMap_Solid_CHEMKIN::sMolar_Species_MixtureAveraged_From_MoleFractions(OpenSMOKE::OpenSMOKEVectorDouble& s_species, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::sMolar_Species_MixtureAveraged_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}
 
	void ThermodynamicsMap_Solid_CHEMKIN::uMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& u_species)
	{
		h_over_RT();
		Add(species_h_over_RT_, -1., &u_species);
		Product(PhysicalConstants::R_J_kmol*this->T_, &u_species);
	}
         
	void ThermodynamicsMap_Solid_CHEMKIN::uMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& u_solidspecies)
	{
		uMolar_Species(aux_vector_total_number_species_);
        for (unsigned int j=1;j<=number_of_solid_species_;j++)
			u_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}        

	void ThermodynamicsMap_Solid_CHEMKIN::gMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& g_species)
	{
		h_over_RT();
		s_over_R();
		Sub(species_h_over_RT_, species_s_over_R_, &g_species);
		Product(PhysicalConstants::R_J_kmol*this->T_, &g_species);
	}
        
	void ThermodynamicsMap_Solid_CHEMKIN::gMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& g_solidspecies)
	{
		gMolar_Species(aux_vector_total_number_species_);
                for (unsigned int j=1;j<=number_of_solid_species_;j++)
                    g_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}

	void ThermodynamicsMap_Solid_CHEMKIN::gMolar_Species_MixtureAveraged_From_MoleFractions(OpenSMOKE::OpenSMOKEVectorDouble& g_species, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::gMolar_Species_MixtureAveraged_From_MoleFractions", "Function not available for Solid thermodynamics!");
	}

	void ThermodynamicsMap_Solid_CHEMKIN::aMolar_Species(OpenSMOKE::OpenSMOKEVectorDouble& a_species)
	{
		h_over_RT();
		s_over_R();
		Sub(species_h_over_RT_, species_s_over_R_, &a_species);
		a_species -= 1.;
		Product(PhysicalConstants::R_J_kmol*this->T_, &a_species);
	}
        
	void ThermodynamicsMap_Solid_CHEMKIN::aMolar_SolidSpecies(OpenSMOKE::OpenSMOKEVectorDouble& a_solidspecies)
	{
		aMolar_Species(aux_vector_total_number_species_);
                for (unsigned int j=1;j<=number_of_solid_species_;j++)
                    a_solidspecies[j] = aux_vector_total_number_species_[number_of_gas_species_+j];
	}        

	void ThermodynamicsMap_Solid_CHEMKIN::aMolar_Species_MixtureAveraged_From_MoleFractions(OpenSMOKE::OpenSMOKEVectorDouble& a_species, const OpenSMOKE::OpenSMOKEVectorDouble& x)
	{
		gMolar_Species_MixtureAveraged_From_MoleFractions(a_species, x);
		a_species -= PhysicalConstants::R_J_kmol*this->T_;
	}

	void ThermodynamicsMap_Solid_CHEMKIN::DerivativesOfConcentrationsWithRespectToMassFractions(const double cTot, const double MW, const OpenSMOKE::OpenSMOKEVectorDouble& omega, OpenSMOKE::OpenSMOKEMatrixDouble* dc_over_omega)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::DerivativesOfConcentrationsWithRespectToMassFractions", "Function not available for Solid thermodynamics!");
	}

	void ThermodynamicsMap_Solid_CHEMKIN::Test(const int nLoops, const double& T, int* index)
	{
		ErrorMessage("ThermodynamicsMap_Solid_CHEMKIN::Test", "Function not available for Solid thermodynamics!");
	}

}

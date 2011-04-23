///
/// @file
/// @brief This is the brief (one line only) description of the function of this file. 
///
/// @details This is the detailed description of the funtion of this file
///
/// @section notes Implementation Notes
/// This is a list of other notes, not related to functionality but rather to implementation. 
/// Also include references, formulae and other notes here.
///
/// @section License
/// Use of this code, either in source or compiled form, is subject to license from the authors.
/// Copyright \htmlonly &copy \endhtmlonly Richard Evans, 2009-2010. All Rights Reserved.
///
/// @section info File Information
/// @author  Richard Evans, rfle500@york.ac.uk
/// @version 1.0
/// @date    11/01/2010
/// @internal
///	Created:		11/01/2010
///	Revision:	  ---
///=====================================================================================
///

// Headers
#include "errors.hpp"
#include "demag.hpp"
#include "voronoi.hpp"
#include "material.hpp"
//#include "multilayer.hpp"
#include "sim.hpp"
#include "random.hpp"
#include "vio.hpp"
#include "vmath.hpp"
#include "vmpi.hpp"

#include <cmath>
#include <iostream>
//==========================================================
// Namespace material_parameters
//==========================================================
namespace mp{
	//----------------------------------
	// Material Container
	//----------------------------------

	//const int max_materials=100;

	int num_materials=1;


	std::vector <materials_t> material(1);


	
	//----------------------------------
	//Input Integration parameters
	//----------------------------------
	double dt_SI;
	double gamma_SI = 1.76E11;
	
	//----------------------------------
	//Derived Integration parameters
	//----------------------------------
	double dt;
	double half_dt;
	
	//----------------------------------
	//Input System Parameters
	//----------------------------------
	int particle_creation_parity;	// Offset of particle centre (odd/even)
	int int_system_dimensions[3];
	double system_dimensions[3];	// Size of system (A)
	double particle_scale;			// Diameter of particles/grains (A)
	double particle_spacing;		// Spacing Between particles (A)
	
	//----------------------------------
	//Derived System Parameters
	//----------------------------------
	double lattice_constant[3];
	double lattice_space_conversion[3];
	string crystal_structure;
	bool single_spin=false;
	string hamiltonian_type; 	// generic
								// LR_FePt
								// SR_FePt
								// LR_Co
								// LR_Fe
								// LR_Ni
								
	string atomic_element[4]; // Different atomic species
	
	int num_nearest_neighbours = 0;
	int hamiltonian_num_neighbours = 0;
	
	//----------------------------------
	// System creation flags
	//----------------------------------
	
	int system_creation_flags[10];
	
///
/// @brief Function to initialise program variables prior to system creation.
///
/// @section License
/// Use of this code, either in source or compiled form, is subject to license from the authors.
/// Copyright \htmlonly &copy \endhtmlonly Richard Evans, 2009-2010. All Rights Reserved.
///
/// @section Information
/// @author  Richard Evans, rfle500@york.ac.uk
/// @version 1.0
/// @date    19/01/2010
///
/// @param[in] infile Main input file name for system initialisation
/// @return EXIT_SUCCESS
///
/// @internal
///	Created:		19/01/2010
///	Revision:	  ---
///=====================================================================================
///
int initialise(std::string const infile){
	//----------------------------------------------------------
	// check calling of routine if error checking is activated
	//----------------------------------------------------------
	if(err::check==true){std::cout << "initialise_variables has been called" << std::endl;}

	if(vmpi::my_rank==0){
		std::cout << "================================================================================" << std::endl;
		std::cout << " " << std::endl;
		std::cout << "Initialising system variables" << std::endl;
	}
	
	// Setup default system settings
	mp::default_system();
	
	// Read values from input files
	int iostat = vin::read(infile);
	if(iostat==EXIT_FAILURE){
		std::cerr << "Error - input file \'" << infile << "\' not found, exiting" << std::endl;
		err::vexit();
	}
	
	// Print out material properties
	//mp::material[0].print();

	// Check for keyword parameter overide
	if(mp::single_spin==true){
		mp::single_spin_system();
	}
	
	// Set derived system parameters
	mp::set_derived_parameters();
	
	// Return
	return EXIT_SUCCESS;
}

int default_system(){

	// Initialise system creation flags to zero
	for (int i=0;i<10;i++){
		mp::system_creation_flags[i] = 0;
		sim::hamiltonian_simulation_flags[i] = 0; 
	}
	
	// Set system dimensions !Angstroms
	mp::lattice_constant[0] = 3.0;
	mp::lattice_constant[1] = 3.0;
	mp::lattice_constant[2] = 3.0;

	mp::system_dimensions[0] = 100.0;
	mp::system_dimensions[1] = 100.0;
	mp::system_dimensions[2] = 100.0;

	mp::particle_scale   = 50.0;
	mp::particle_spacing = 10.0;
	
	mp::particle_creation_parity=0;
	mp::crystal_structure = "sc";
	mp::hamiltonian_type = "generic";

	// Voronoi Variables
	create_voronoi::voronoi_sd=0.1;
	create_voronoi::parity=0;
	
	// Setup Hamiltonian Flags
	sim::hamiltonian_simulation_flags[0] = 1;	// Exchange
	sim::hamiltonian_simulation_flags[1] = 1;	// Anisotropy
	sim::hamiltonian_simulation_flags[2] = 1;	// Applied
	sim::hamiltonian_simulation_flags[3] = 1;	// Thermal
	sim::hamiltonian_simulation_flags[4] = 0;	// Dipolar

	// Setup Simulation Variables
	sim::total_time = 1000000;			// total simulation time (single run)
	sim::loop_time = 0;			// time in loop, eg hysteresis, Tc
	sim::partial_time=100;			// time between statistics collection
	sim::equilibration_time=100000;	// time for equilibration before main loop
	sim::temperature = 0.0;	// Constant system temperature

	// demag variables
	//demag::demag_resolution=2;
	//demag::update_rate=10000;
	
	//Integration parameters
	dt_SI = 1.0e-15;	// seconds
	dt = dt_SI*mp::gamma_SI; // Must be set before Hth
	half_dt = 0.5*dt;

	// MPI Mode (Assume decomposition)
	//vmpi::mpi_mode=1;
	//mpi_create_variables::mpi_interaction_range=2; // Unit cells
	//mpi_create_variables::mpi_comms_identify=true;

	//------------------------------------------------------------------------------
	// Material Definitions
	//------------------------------------------------------------------------------
	num_materials=1;
	material.resize(num_materials);

	//-------------------------------------------------------
	// Material 0
	//-------------------------------------------------------
	material[0].name="Co";
	material[0].alpha=0.1;
	material[0].Jij_matrix_SI[0]=-11.2e-21;
	material[0].mu_s_SI=1.5*9.27400915e-24;
	material[0].Ku1_SI=-4.644e-24;
	material[0].gamma_rel=1.0;
	material[0].hamiltonian_type="generic";
	material[0].element="Ag ";

	// Disable Error Checking
	err::check=false;
	
	// Initialise random number generator
	mtrandom::grnd.seed(1234);

	return EXIT_SUCCESS;
}

int single_spin_system(){

	// Reset system creation flags to zero
	for (int i=0;i<10;i++){
		mp::system_creation_flags[i] = 0;
	}
	
	// Set system dimensions !Angstroms
	mp::lattice_constant[0] = 3.0;
	mp::lattice_constant[1] = 3.0;
	mp::lattice_constant[2] = 3.0;

	mp::system_dimensions[0] = 2.0;
	mp::system_dimensions[1] = 2.0;
	mp::system_dimensions[2] = 2.0;

	mp::particle_scale   = 50.0;
	mp::particle_spacing = 10.0;
	
	mp::particle_creation_parity=0;
	mp::crystal_structure = "sc";
	mp::hamiltonian_type = "generic";
	
	// Turn off multi-spin Flags
	sim::hamiltonian_simulation_flags[0] = 0;	// Exchange
	sim::hamiltonian_simulation_flags[4] = 0;	// Dipolar

	// MPI Mode (Homogeneous execution)
	//vmpi::mpi_mode=0;
	//mpi_create_variables::mpi_interaction_range=2; // Unit cells
	//mpi_create_variables::mpi_comms_identify=false;

	return EXIT_SUCCESS;
}

int set_derived_parameters(){

	//----------------------------------
	//Derived System Parameters
	//----------------------------------
	mp::lattice_space_conversion[0] = mp::lattice_constant[0]*0.5;
	mp::lattice_space_conversion[1] = mp::lattice_constant[1]*0.5*0.333333333333333;
	mp::lattice_space_conversion[2] = mp::lattice_constant[2]*0.5;

	mp::int_system_dimensions[0] = 2*vmath::iround(mp::system_dimensions[0]/mp::lattice_constant[0]);
	mp::int_system_dimensions[1] = 6*vmath::iround(mp::system_dimensions[1]/mp::lattice_constant[1]);
	mp::int_system_dimensions[2] = 2*vmath::iround(mp::system_dimensions[2]/mp::lattice_constant[2]);
		
	double num_atoms_per_unit_cell=0; 
	
	if(mp::crystal_structure=="sc"){
		mp::num_nearest_neighbours = 6;
		num_atoms_per_unit_cell=1.0;
	}
	else if(mp::crystal_structure=="bcc"){
		mp::num_nearest_neighbours = 8;
		num_atoms_per_unit_cell=2.0;
	}
	else if(mp::crystal_structure=="fct"){
		mp::num_nearest_neighbours = 4;
		num_atoms_per_unit_cell=2.0;
	}
	else if(mp::crystal_structure=="fcc"){
		mp::num_nearest_neighbours = 12;
		num_atoms_per_unit_cell=4.0;
	}
	else{
		 std::cout << "Error in determining num_nearest_neighbours - unknown crystal type \'" << mp::crystal_structure << "\'" << std::endl;
		 err::vexit();
	}
	
	if(mp::hamiltonian_type=="generic")	mp::hamiltonian_num_neighbours = mp::num_nearest_neighbours;
	if(mp::hamiltonian_num_neighbours==0){
		 std::cout << "Error in determining hamiltonian_num_neighbours - unknown Hamiltonian type \'" << mp::hamiltonian_type << "\'" << std::endl;
		 err::vexit();
	}

	// Set integration constants
	mp::dt = mp::dt_SI*mp::gamma_SI; // Must be set before Hth
	mp::half_dt = 0.5*mp::dt;

	// Ensure H vector is unit length
	double mod_H=1.0/sqrt(sim::H_vec[0]*sim::H_vec[0]+sim::H_vec[1]*sim::H_vec[1]+sim::H_vec[2]*sim::H_vec[2]);
	sim::H_vec[0]*=mod_H;
	sim::H_vec[1]*=mod_H;
	sim::H_vec[2]*=mod_H;

	// Calculate moment, magnetisation, and anisotropy constants
	for(int mat=0;mat<mp::num_materials;mat++){
		double V=mp::lattice_constant[0]*mp::lattice_constant[1]*mp::lattice_constant[2];
		// Set magnetisation from mu_s and a
		if(material[mat].moment_flag==true){
			material[mat].magnetisation=num_atoms_per_unit_cell*material[mat].mu_s_SI/V;
		}
		// Set mu_s from magnetisation and a
		else {
			material[mat].mu_s_SI=material[mat].magnetisation*V/num_atoms_per_unit_cell;
		}
		// Set K as energy/atom
		if(material[mat].anis_flag==false){
			material[mat].Ku1_SI=material[mat].Ku1_SI*V/num_atoms_per_unit_cell;
			std::cout << "setting " << material[mat].Ku1_SI << std::endl;
		}
	}
	const string blank="";
	// Set derived material parameters
	for(int mat=0;mat<mp::num_materials;mat++){
		mp::material[mat].hamiltonian_type="generic";
		mp::material[mat].one_oneplusalpha_sq			=-mp::material[mat].gamma_rel/(1.0+mp::material[mat].alpha*mp::material[mat].alpha);
		mp::material[mat].alpha_oneplusalpha_sq			= mp::material[mat].alpha*mp::material[mat].one_oneplusalpha_sq;
		
		// set initial spins to unit length
		double sx = mp::material[mat].initial_spin[0];
		double sy = mp::material[mat].initial_spin[1];
		double sz = mp::material[mat].initial_spin[2];

		double modS = 1.0/sqrt(sx*sx+sy*sy+sz*sz);
		mp::material[mat].initial_spin[0]*=modS;
		mp::material[mat].initial_spin[1]*=modS;
		mp::material[mat].initial_spin[2]*=modS;
			
		for(int j=0;j<mp::num_materials;j++){
			material[mat].Jij_matrix[j]				= mp::material[mat].Jij_matrix_SI[j]/mp::material[mat].mu_s_SI;
		}
		mp::material[mat].Ku									= mp::material[mat].Ku1_SI/mp::material[mat].mu_s_SI;
		mp::material[mat].H_th_sigma						= sqrt(2.0*mp::material[mat].alpha*1.3806503e-23/
																  (mp::material[mat].mu_s_SI*mp::material[mat].gamma_rel*dt));
		// If local crystal is unset, use global type
		if(mp::material[mat].crystal_structure==blank){
			mp::material[mat].crystal_structure=mp::crystal_structure;
		}
		// calculate number of neighbours for each material
		if(mp::material[mat].hamiltonian_type=="generic"){
			if(mp::material[mat].crystal_structure=="sc"){
				mp::material[mat].num_nearest_neighbours		= 6;
				mp::material[mat].hamiltonian_num_neighbours	= 6;
				mp::material[mat].cutoff = 1.01;
			}
			else if(mp::material[mat].crystal_structure=="bcc"){
				mp::material[mat].num_nearest_neighbours		= 8;
				mp::material[mat].hamiltonian_num_neighbours	= 8;
				mp::material[mat].cutoff = sqrt(3.0)*0.5*1.01;

			}
			else if(mp::material[mat].crystal_structure=="fcc"){
				mp::material[mat].num_nearest_neighbours		= 12;
				mp::material[mat].hamiltonian_num_neighbours	= 12;
				mp::material[mat].cutoff = sqrt(2.0)*0.5*1.01;

			}
			else{
				std::cerr << "Error in determining num_nearest_neighbours - unknown crystal type \'";
				std::cerr << mp::material[mat].crystal_structure << "\'" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		else{
			std::cerr << "Error, only generic hamiltonians are implemented at present, exiting" << std::endl;
			exit(EXIT_FAILURE);
		}
		
		
		//std::cout << "checking range exclusivity" << std::endl;
		// Check for exclusivity of range
		if(material[mat].geometry!=0){
			const double lmin=material[mat].min;
			const double lmax=material[mat].max;
			for(int nmat=0;nmat<mp::num_materials;nmat++){
				if(nmat!=mat){
					double min=material[nmat].min;
					double max=material[nmat].max;
					std::cout << lmin << "\t" << min << "\t" << max << std::endl;
					std::cout << lmax << "\t" << min << "\t" << max << std::endl;
					if(((lmin>min) && (lmin<max)) || ((lmax>min) && (lmax<max))){
						std::cerr << "Warning - material " << mat << " overlaps material " << nmat << " - possibly use alloy keyword instead" << std::endl;
						std::cerr << " Material "<< mat << ":min = " << lmin << std::endl;
						std::cerr << " Material "<< mat << ":max = " << lmax << std::endl;
						std::cerr << " Material "<< nmat << ":min = " << min << std::endl;
						std::cerr << " Material "<< nmat << ":max = " << max << std::endl;
						//exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
	
	return EXIT_SUCCESS;
}

} // end of namespace mp

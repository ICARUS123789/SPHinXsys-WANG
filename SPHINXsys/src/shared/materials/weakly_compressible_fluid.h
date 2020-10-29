/* -------------------------------------------------------------------------*
*								SPHinXsys									*
* --------------------------------------------------------------------------*
* SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle	*
* Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
* physical accurate simulation and aims to model coupled industrial dynamic *
* systems including fluid, solid, multi-body dynamics and beyond with SPH	*
* (smoothed particle hydrodynamics), a meshless computational method using	*
* particle discretization.													*
*																			*
* SPHinXsys is partially funded by German Research Foundation				*
* (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1				*
* and HU1527/12-1.															*
*                                                                           *
* Portions copyright (c) 2017-2020 Technical University of Munich and		*
* the authors' affiliations.												*
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License"); you may   *
* not use this file except in compliance with the License. You may obtain a *
* copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
*                                                                           *
* --------------------------------------------------------------------------*/
/**
 * @file 	weakly_compressible_fluid.h
 * @brief 	Describe the weakly compressible fluid which is used 
 * 			model incompressible fluids. Here, we have included several equation of states.
 * 			Futhermore, A typical non-newtonian fluid model is included.  
 * @author  Xiangyu Hu, Luhui Han and Chi Zhang
 * @version 0.1.0
 */

#pragma once

#include "base_material.h"

namespace SPH {

	//----------------------------------------------------------------------
	//		preclaimed classes
	//----------------------------------------------------------------------
	class ViscoelasticFluidParticles;

	/**
	 * @class WeaklyCompressibleFluid
	 * @brief Linear equation of state (EOS).
	 */
	class WeaklyCompressibleFluid : public Fluid
	{
	protected:
		/** reference pressure */
		Real p0_;

		/** assign derived material properties*/
		virtual void assignDerivedMaterialParameters() override {
			Fluid::assignDerivedMaterialParameters();
			p0_ = rho_0_ * c_0_ * c_0_;
		};
	public:
		/** Constructor with material name. */
		explicit WeaklyCompressibleFluid() : Fluid(), p0_(1.0) {
			material_name_ = "WeaklyCompressibleFluid";
		};
		virtual ~WeaklyCompressibleFluid() {};

		/** the interface for dynamical cast*/
		virtual WeaklyCompressibleFluid* pointToThisObject() override { return this; };

		virtual Real GetPressure(Real rho) override;
		virtual Real DensityFromPressure(Real p) override;
		virtual Real GetSoundSpeed(Real p = 0.0, Real rho = 1.0) override;

		/** riemann solver */
		virtual Real RiemannSolverForPressure(Real rhol, Real rhor, Real pl,
			Real pr, Real ul, Real ur) override;
		virtual Real RiemannSolverForVelocity(Real rhol, Real rhor, Real pl,
			Real pr, Real ul, Real ur) override;
	};

	/**
	* @class WeaklyCompressibleFluidFreeSurface
	* @brief Equation of state (EOS) with cut-off pressure.
	*/
	template<class WeaklyCompressibleFluidType>
	class WeaklyCompressibleFluidFreeSurface : public WeaklyCompressibleFluid
	{
	protected:
		WeaklyCompressibleFluidType* fluid_;
		Real cutoff_pressure_, cutoff_density_;

		/** assign derived material properties*/
		virtual void assignDerivedMaterialParameters() {
			WeaklyCompressibleFluid::assignDerivedMaterialParameters();
		};
	public:
		/** constructor */
		WeaklyCompressibleFluidFreeSurface(Real cutoff_pressure)
			: WeaklyCompressibleFluid(),
			cutoff_pressure_(cutoff_pressure) {
			fluid_ = new WeaklyCompressibleFluidType();
			material_name_ = fluid_->material_name_ + "FreeSurface";
			cutoff_density_ = fluid_->DensityFromPressure(cutoff_pressure);
		}; 
		virtual ~WeaklyCompressibleFluidFreeSurface() {};

		virtual Real GetPressure(Real rho) override {
			return rho < cutoff_density_ ? cutoff_pressure_ : fluid_->GetPressure(rho);
		};
	};

	/**
	 * @class SymmetricTaitFluid
	 * @brief Tait EOS for positive and negative pressure symmetrically.
	 */
	class SymmetricTaitFluid : public WeaklyCompressibleFluid
	{
	protected:
		//determine the stiffness of the fluid
		int gamma_;

		/** assign derived material properties*/
		virtual void assignDerivedMaterialParameters() override {
			WeaklyCompressibleFluid::assignDerivedMaterialParameters();
		};

	public:
		/** constructor. */
		SymmetricTaitFluid() : WeaklyCompressibleFluid(), gamma_(2) {
			material_name_ = "SymmetricTaitFluid";
		};
		virtual ~SymmetricTaitFluid() {};

		/** the interface for dynamical cast*/
		virtual SymmetricTaitFluid* pointToThisObject() override { return this; };

		virtual Real GetPressure(Real rho) override;
		virtual Real DensityFromPressure(Real p) override;
		virtual Real GetSoundSpeed(Real p = 0.0, Real rho = 1.0) override;
	};

	/**
	 * @class Oldroyd_B_Fluid
	 * @brief linear EOS with relaxation time and polymetric viscosity.
	 */
	class Oldroyd_B_Fluid : public WeaklyCompressibleFluid
	{
	protected:
		/** relaxation time */
		Real lambda_;
		/** polymeric viscosity */
		Real mu_p_;
		/** particles for this material */
		ViscoelasticFluidParticles* viscoelastic_fluid_particles_;

		/** assign derived material properties*/
		virtual void assignDerivedMaterialParameters() override {
			WeaklyCompressibleFluid::assignDerivedMaterialParameters();
		};
	public:
		/** constructor */
		explicit Oldroyd_B_Fluid() : WeaklyCompressibleFluid(),
			lambda_(1.0), mu_p_(0.0) {
			material_name_ = "Oldroyd_B_Fluid";
		};
		virtual ~Oldroyd_B_Fluid() {};

		/** assign particles to this material */
		void assignViscoelasticFluidParticles(ViscoelasticFluidParticles* viscoelastic_fluid_particles) {
			viscoelastic_fluid_particles_ = viscoelastic_fluid_particles;
		};
		Real getReferenceRelaxationTime() { return lambda_; };
		Real ReferencePolymericViscosity() { return mu_p_; };

		/** the interface for dynamical cast*/
		virtual Oldroyd_B_Fluid* pointToThisObject() override { return this; };
	};
}

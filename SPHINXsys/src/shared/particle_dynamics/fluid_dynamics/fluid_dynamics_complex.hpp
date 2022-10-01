/**
 * @file 	fluid_dynamics_complex.hpp
 * @author	Chi ZHang and Xiangyu Hu
 */

#pragma once

#include "fluid_dynamics_complex.h"

//=================================================================================================//
namespace SPH
{
	//=================================================================================================//
	namespace fluid_dynamics
	{
		//=================================================================================================//
		template <class BaseRelaxationType>
		template <class BaseBodyRelationType>
		RelaxationWithWall<BaseRelaxationType>::
			RelaxationWithWall(BaseBodyRelationType &base_body_relation,
							   BaseBodyRelationContact &wall_contact_relation)
			: BaseRelaxationType(base_body_relation), FluidWallData(wall_contact_relation)
		{
			if (&base_body_relation.sph_body_ != &wall_contact_relation.sph_body_)
			{
				std::cout << "\n Error: the two body_relations do not have the same source body!" << std::endl;
				std::cout << __FILE__ << ':' << __LINE__ << std::endl;
				exit(1);
			}

			for (size_t k = 0; k != FluidWallData::contact_particles_.size(); ++k)
			{
				Real rho0_k = FluidWallData::contact_particles_[k]->rho0_;
				wall_inv_rho0_.push_back(1.0 / rho0_k);
				wall_mass_.push_back(&(FluidWallData::contact_particles_[k]->mass_));
				wall_vel_ave_.push_back(FluidWallData::contact_particles_[k]->AverageVelocity());
				wall_acc_ave_.push_back(FluidWallData::contact_particles_[k]->AverageAcceleration());
				wall_n_.push_back(&(FluidWallData::contact_particles_[k]->n_));
			}
		}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		template <typename... Args>
		DensitySummation<DensitySummationInnerType>::DensitySummation(Args &&...args)
			: BaseInteractionDynamicsComplex<DensitySummationInnerType, FluidContactData>(
				  std::forward<Args>(args)...)
		{
			for (size_t k = 0; k != this->contact_particles_.size(); ++k)
			{
				Real rho0_k = this->contact_particles_[k]->rho0_;
				contact_inv_rho0_.push_back(1.0 / rho0_k);
				contact_mass_.push_back(&(this->contact_particles_[k]->mass_));
			}
		};
		//=================================================================================================//
		template <class DensitySummationInnerType>
		void DensitySummation<DensitySummationInnerType>::interaction(size_t index_i, Real dt)
		{
			DensitySummationInnerType::interaction(index_i, dt);

			/** Contact interaction. */
			Real sigma(0.0);
			Real inv_Vol_0_i = this->rho0_ / this->mass_[index_i];
			for (size_t k = 0; k < this->contact_configuration_.size(); ++k)
			{
				StdLargeVec<Real> &contact_mass_k = *(this->contact_mass_[k]);
				Real contact_inv_rho0_k = contact_inv_rho0_[k];
				Neighborhood &contact_neighborhood = (*this->contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					sigma += contact_neighborhood.W_ij_[n] * inv_Vol_0_i * contact_inv_rho0_k * contact_mass_k[contact_neighborhood.j_[n]];
				}
			}
			this->rho_sum_[index_i] += sigma * this->rho0_ * this->inv_sigma0_;
		}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		template <class BaseBodyRelationType>
		BaseViscousAccelerationWithWall<ViscousAccelerationInnerType>::
			BaseViscousAccelerationWithWall(BaseBodyRelationType &base_body_relation,
							BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<ViscousAccelerationInnerType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		void BaseViscousAccelerationWithWall<ViscousAccelerationInnerType>::interaction(size_t index_i, Real dt)
		{
			ViscousAccelerationInnerType::interaction(index_i, dt);

			Real rho_i = this->rho_[index_i];
			const Vecd &vel_i = this->vel_[index_i];

			Vecd acceleration(0), vel_derivative(0);
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				Neighborhood &contact_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					size_t index_j = contact_neighborhood.j_[n];
					Real r_ij = contact_neighborhood.r_ij_[n];

					vel_derivative = 2.0 * (vel_i - vel_ave_k[index_j]) / (r_ij + 0.01 * this->smoothing_length_);
					acceleration += 2.0 * this->mu_ * vel_derivative * contact_neighborhood.dW_ijV_j_[n] / rho_i;
				}
			}

			this->acc_prior_[index_i] += acceleration;
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		template <class BaseBodyRelationType>
		BasePressureRelaxationWithWall<BasePressureRelaxationType>::
			BasePressureRelaxationWithWall(BaseBodyRelationType &base_body_relation,
							   BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<BasePressureRelaxationType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void BasePressureRelaxationWithWall<BasePressureRelaxationType>::interaction(size_t index_i, Real dt)
		{
			BasePressureRelaxationType::interaction(index_i, dt);

			Vecd acc_prior_i = computeNonConservativeAcceleration(index_i);

			Vecd acceleration(0.0);
			Real rho_dissipation(0);
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				StdLargeVec<Vecd> &acc_ave_k = *(this->wall_acc_ave_[k]);
				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real dW_ijV_j = wall_neighborhood.dW_ijV_j_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];

					Real face_wall_external_acceleration = dot((acc_prior_i - acc_ave_k[index_j]), -e_ij);
					Real p_in_wall = this->p_[index_i] + this->rho_[index_i] * r_ij * SMAX(0.0, face_wall_external_acceleration);
					acceleration -= (this->p_[index_i] + p_in_wall) * e_ij * dW_ijV_j;
					rho_dissipation += this->riemann_solver_.DissipativeUJump(this->p_[index_i] - p_in_wall) * dW_ijV_j;
				}
			}
			this->acc_[index_i] += acceleration / this->rho_[index_i];
			this->drho_dt_[index_i] += 0.5 * rho_dissipation * this->rho_[index_i];
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		Vecd BasePressureRelaxationWithWall<BasePressureRelaxationType>::computeNonConservativeAcceleration(size_t index_i)
		{
			return this->acc_prior_[index_i];
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		template <class BaseBodyRelationType>
		BaseExtendPressureRelaxationWithWall<BasePressureRelaxationType>::
			BaseExtendPressureRelaxationWithWall(BaseBodyRelationType &base_body_relation,
									 BaseBodyRelationContact &wall_contact_relation, Real penalty_strength)
			: BasePressureRelaxationWithWall<BasePressureRelaxationType>(base_body_relation, wall_contact_relation),
			  penalty_strength_(penalty_strength)
		{
			this->particles_->registerVariable(non_cnsrv_acc_, "NonConservativeAcceleration");
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void BaseExtendPressureRelaxationWithWall<BasePressureRelaxationType>::initialization(size_t index_i, Real dt)
		{
			BasePressureRelaxationType::initialization(index_i, dt);
			non_cnsrv_acc_[index_i] = Vecd(0);
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void BaseExtendPressureRelaxationWithWall<BasePressureRelaxationType>::interaction(size_t index_i, Real dt)
		{
			BasePressureRelaxationWithWall<BasePressureRelaxationType>::interaction(index_i, dt);

			Real rho_i = this->rho_[index_i];
			Real penalty_pressure = this->p_[index_i];
			Vecd acceleration(0);
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				Real particle_spacing_j1 = 1.0 / FluidWallData::contact_bodies_[k]->sph_adaptation_->ReferenceSpacing();
				Real particle_spacing_ratio2 = 1.0 / (this->sph_body_.sph_adaptation_->ReferenceSpacing() * particle_spacing_j1);
				particle_spacing_ratio2 *= 0.1 * particle_spacing_ratio2;

				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real dW_ijV_j = wall_neighborhood.dW_ijV_j_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];
					Vecd &n_j = n_k[index_j];

					/** penalty method to prevent particle running into boundary */
					Real projection = dot(e_ij, n_j);
					Real delta = 2.0 * projection * r_ij * particle_spacing_j1;
					Real beta = delta < 1.0 ? (1.0 - delta) * (1.0 - delta) * particle_spacing_ratio2 : 0.0;
					// penalty must be positive so that the penalty force is pointed to fluid inner domain
					Real penalty = penalty_strength_ * beta * fabs(projection * penalty_pressure);

					// penalty force induced acceleration
					acceleration -= 2.0 * penalty * n_j * dW_ijV_j / rho_i;
				}
			}
			this->acc_[index_i] += acceleration;
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		Vecd BaseExtendPressureRelaxationWithWall<BasePressureRelaxationType>::
			computeNonConservativeAcceleration(size_t index_i)
		{
			Vecd acceleration = BasePressureRelaxationType::computeNonConservativeAcceleration(index_i);
			non_cnsrv_acc_[index_i] = acceleration;
			return acceleration;
		}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		template <class BaseBodyRelationType>
		BaseDensityRelaxationWithWall<BaseDensityRelaxationType>::
			BaseDensityRelaxationWithWall(BaseBodyRelationType &base_body_relation,
							  BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<BaseDensityRelaxationType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		void BaseDensityRelaxationWithWall<BaseDensityRelaxationType>::interaction(size_t index_i, Real dt)
		{
			BaseDensityRelaxationType::interaction(index_i, dt);

			Real density_change_rate = 0.0;
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				Vecd &acc_prior_i = this->acc_prior_[index_i];

				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				StdLargeVec<Vecd> &acc_ave_k = *(this->wall_acc_ave_[k]);
				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];
					Real dW_ijV_j = wall_neighborhood.dW_ijV_j_[n];

					Vecd vel_in_wall = 2.0 * vel_ave_k[index_j] - this->vel_[index_i];
					density_change_rate += dot(this->vel_[index_i] - vel_in_wall, e_ij) * dW_ijV_j;
				}
			}
			this->drho_dt_[index_i] += density_change_rate * this->rho_[index_i];
		}
		//=================================================================================================//
	}
	//=================================================================================================//
}
//=================================================================================================//
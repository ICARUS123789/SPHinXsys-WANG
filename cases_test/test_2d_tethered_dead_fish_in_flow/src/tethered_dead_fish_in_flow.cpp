/**
* @file    tethered_dead_fish_in_flow.cpp
* @brief   fish flapping passively in flow
* @author  Xiangyu Hu and Chi Zhang
*/
#include "sphinxsys.h"
/**
* Create the shapes for fish and bones.
*/
#include "fish_and_bones.h"
/**
* @brief Namespace cite here.
*/
using namespace SPH;
/**
* @brief Basic geometry parameters and numerical setup.
*/
Real DL = 11.0;                             /**< Channel length. */
Real DH = 8.0;                              /**< Channel height. */
Real resolution_ref = 0.1;           /** Initial particle spacing. */
Real DL_sponge = resolution_ref * 20.0; /**< Sponge region to impose inflow condition. */
Real BW = resolution_ref * 4.0;        /**< Extending width for BCs. */
/** Domain bounds of the system. */
BoundingBox system_domain_bounds(Vec2d(-DL_sponge - BW, -BW), Vec2d(DL + BW, DH + BW));

Real cx = 2.0;                              /**< Center of fish in x direction. */
Real cy = 4.0;                              /**< Center of fish in y direction. */
Real fish_length = 3.738;                   /**< Length of fish. */
Vec3d tethering_point(-1.0, cy, 0.0);        /**< The tethering point. */
/**
 * Material properties of the fluid.
 */
Real rho0_f = 1.0;
Real U_f = 1.0;
Real c_f = 10.0 * U_f;
Real Re = 5.0e3;
Real mu_f = rho0_f * U_f * (fish_length) / Re;
/**
 * Material properties of the fish body.
 */
Real rho0_s = 1.0;
Real poisson = 0.49;
Real Ae = 2.0e2;
Real Youngs_modulus = Ae * rho0_f * U_f * U_f;
/**
 * Basic geometries for construct SPH bodies.
 */
/**
* @brief create a water block shape
*/
std::vector<Vecd> CreatWaterBlockShape()
{
	std::vector<Vecd> pnts_shaping_water_block;
	pnts_shaping_water_block.push_back(Vecd(-DL_sponge, 0.0));
	pnts_shaping_water_block.push_back(Vecd(-DL_sponge, DH));
	pnts_shaping_water_block.push_back(Vecd(DL, DH));
	pnts_shaping_water_block.push_back(Vecd(DL, 0.0));
	pnts_shaping_water_block.push_back(Vecd(-DL_sponge, 0.0));

	return pnts_shaping_water_block;
}
/**
* @brief create a buffer for water block shape
*/
std::vector<Vecd> CreatInflowBufferShape()
{
	std::vector<Vecd> pnts_buffer;
	pnts_buffer.push_back(Vecd(-DL_sponge, 0.0));
	pnts_buffer.push_back(Vecd(-DL_sponge, DH));
	pnts_buffer.push_back(Vecd(0.0, DH));
	pnts_buffer.push_back(Vecd(0.0, 0.0));
	pnts_buffer.push_back(Vecd(-DL_sponge, 0.0));

	return pnts_buffer;
}
/**
* @brief create outer wall shape
*/
std::vector<Vecd> CreatOuterWallShape()
{
	std::vector<Vecd> pnts_shaping_outer_wall;
	pnts_shaping_outer_wall.push_back(Vecd(-DL_sponge - BW, -BW));
	pnts_shaping_outer_wall.push_back(Vecd(-DL_sponge - BW, DH + BW));
	pnts_shaping_outer_wall.push_back(Vecd(DL + BW, DH + BW));
	pnts_shaping_outer_wall.push_back(Vecd(DL + BW, -BW));
	pnts_shaping_outer_wall.push_back(Vecd(-DL_sponge - BW, -BW));

	return pnts_shaping_outer_wall;
}
/**
* @brief create inner wall shape
*/
std::vector<Vecd> CreatInnerWallShape()
{
	std::vector<Vecd> pnts_shaping_inner_wall;
	pnts_shaping_inner_wall.push_back(Vecd(-DL_sponge - 2.0 * BW, 0.0));
	pnts_shaping_inner_wall.push_back(Vecd(-DL_sponge - 2.0 * BW, DH));
	pnts_shaping_inner_wall.push_back(Vecd(DL + 2.0 * BW, DH));
	pnts_shaping_inner_wall.push_back(Vecd(DL + 2.0 * BW, 0.0));
	pnts_shaping_inner_wall.push_back(Vecd(-DL_sponge - 2.0 * BW, 0.0));

	return pnts_shaping_inner_wall;
}
/**
* @brief create blocking shape to separate fish head out
*/
Real head_size = 1.0;
std::vector<Vecd> CreatFishBlockingShape()
{
	std::vector<Vecd> pnts_blocking_shape;
	pnts_blocking_shape.push_back(Vecd(cx + head_size, cy - 0.4));
	pnts_blocking_shape.push_back(Vecd(cx + head_size, cy + 0.4));
	pnts_blocking_shape.push_back(Vecd(cx + 5.0, cy + 0.4));
	pnts_blocking_shape.push_back(Vecd(cx + 5.0, cy - 0.4));
	pnts_blocking_shape.push_back(Vecd(cx + head_size, cy - 0.4));

	return pnts_blocking_shape;
}
/**
* Water body defintion.
*/
class WaterBlock : public FluidBody
{
public:
	WaterBlock(SPHSystem& system, string body_name)
		: FluidBody(system, body_name)
	{
		std::vector<Vecd> water_block_shape = CreatWaterBlockShape();
		body_shape_ = new ComplexShape(body_name);
		body_shape_->addAPolygon(water_block_shape, ShapeBooleanOps::add);
		/** Exclude the fish body. */
		std::vector<Vecd> fish_shape 
			= CreatFishShape(cx, cy, fish_length, particle_adaptation_->ReferenceSpacing() * 0.5);
		body_shape_->addAPolygon(fish_shape, ShapeBooleanOps::sub);
	}
};
/**
 * @brief 	Case dependent material properties definition.
 */
class WaterMaterial : public SymmetricTaitFluid
{
public:
	WaterMaterial() : SymmetricTaitFluid()
	{
		rho_0_ = rho0_f;
		c_0_ = c_f;
		mu_ = mu_f;

		assignDerivedMaterialParameters();
	}
};
/**
* Solid wall.
*/
class WallBoundary : public SolidBody
{
public:
	WallBoundary(SPHSystem& system, string body_name)
		: SolidBody(system, body_name)
	{
		std::vector<Vecd> outer_wall_shape = CreatOuterWallShape();
		std::vector<Vecd> inner_wall_shape = CreatInnerWallShape();
		body_shape_ = new ComplexShape(body_name);
		body_shape_->addAPolygon(outer_wall_shape, ShapeBooleanOps::add);
		body_shape_->addAPolygon(inner_wall_shape, ShapeBooleanOps::sub);
	}
};
/**
* Fish body with tethering constraint.
*/
class FishBody : public SolidBody
{

public:
	FishBody(SPHSystem& system, string body_name)
		: SolidBody(system, body_name, new ParticleAdaptation(1.15, 1))
	{
		std::vector<Vecd> fish_shape 
			= CreatFishShape(cx, cy, fish_length, particle_adaptation_->ReferenceSpacing());
		ComplexShape original_body_shape;
		original_body_shape.addAPolygon(fish_shape, ShapeBooleanOps::add);
		body_shape_ = new LevelSetComplexShape(this, original_body_shape);

	}
};
/**
*@brief Define gate material.
*/
class FishMaterial : public NeoHookeanSolid
{
public:
	FishMaterial() : NeoHookeanSolid()
	{
		rho_0_ = rho0_s;
		E_0_ = Youngs_modulus;
		nu_ = poisson;

		assignDerivedMaterialParameters();
	}
};

/**
* @brief create fish head for constraint
*/
class FishHead : public SolidBodyPartForSimbody
{
public:
	FishHead(SolidBody* solid_body,
		string constrained_region_name, Real solid_body_density)
		: SolidBodyPartForSimbody(solid_body,
			constrained_region_name)
	{
		//geometry
		std::vector<Vecd> fish_shape 
			= CreatFishShape(cx, cy, fish_length, body_->particle_adaptation_->ReferenceSpacing());
		std::vector<Vecd> fish_blocking_shape = CreatFishBlockingShape();
		body_part_shape_ = new ComplexShape(constrained_region_name);
		body_part_shape_->addAPolygon(fish_shape, ShapeBooleanOps::add);
		body_part_shape_->addAPolygon(fish_blocking_shape, ShapeBooleanOps::sub);

		//tag the constrained particle
		tagBodyPart();
	}
};
/**
* @brief inflow buffer
*/
class InflowBuffer : public BodyPartByCell
{
public:
	InflowBuffer(FluidBody* fluid_body, string constrained_region_name)
		: BodyPartByCell(fluid_body, constrained_region_name)
	{
		/** Geomtry definition. */
		std::vector<Vecd> inflow_shape = CreatInflowBufferShape();
		body_part_shape_ = new ComplexShape(constrained_region_name);
		body_part_shape_->addAPolygon(inflow_shape, ShapeBooleanOps::add);

		//tag the constrained particle
		tagBodyPart();
	}
};
/**
* Definition of an observer body with several particle to record particular property.
*/
class Observer : public FictitiousBody
{
public:
	Observer(SPHSystem& system, string body_name)
		: FictitiousBody(system, body_name, new ParticleAdaptation(1.15, 1))
	{
		/** postion and volume. */
		body_input_points_volumes_.push_back(make_pair(Vecd(cx + resolution_ref, cy), 0.0));
		body_input_points_volumes_.push_back(make_pair(Vecd(cx + fish_length - resolution_ref, cy), 0.0));
	}
};
/**
* Inflow boundary condition.
*/
class ParabolicInflow : public fluid_dynamics::InflowBoundaryCondition
{
	Real u_ave_, u_ref_, t_ref;

public:
	ParabolicInflow(FluidBody* fluid_body,
		BodyPartByCell* constrained_region)
		: InflowBoundaryCondition(fluid_body, constrained_region)
	{
		u_ave_ = 0.0;
		u_ref_ = 1.0;
		t_ref = 4.0;
	}

	Vecd getTargetVelocity(Vecd& position, Vecd& velocity)
	{
		Real u = velocity[0];
		Real v = velocity[1];
		if (position[0] < 0.0) {
			u = 6.0 * u_ave_ * position[1] * (DH - position[1]) / DH / DH;
			v = 0.0;
		}
		return Vecd(u, v);
	}

	void setupDynamics(Real dt = 0.0) override
	{
		Real run_time = GlobalStaticVariables::physical_time_;
		u_ave_ = run_time < t_ref ? 0.5 * u_ref_ * (1.0 - cos(Pi * run_time / t_ref)) : u_ref_;
	}
};
/**
* Main program starts here.
*/
int main()
{
	/**
	* Build up context -- a SPHSystem.
	*/
	SPHSystem system(system_domain_bounds, resolution_ref);
	/** Tag for run particle relaxation for the initial body fitted distribution. */
	system.run_particle_relaxation_ = false;
	/** Tag for computation start with relaxed body fitted particles distribution. */
	system.reload_particles_ = true;
	/** Tag for computation from restart files. 0: start with initial condition. */
	system.restart_step_ = 0;
	/**
	* @brief   Particles and body creation for water.
	*/
	WaterBlock* water_block = new WaterBlock(system, "WaterBody");
	WaterMaterial    *water_fluid = new WaterMaterial();
	FluidParticles    fluid_particles(water_block, water_fluid);
	/**
	* @brief   Particles and body creation for wall boundary.
	*/
	WallBoundary* wall_boundary = new   WallBoundary(system, "Wall");
	SolidParticles	wall_particles(wall_boundary);
	/**
	* @brief   Particles and body creation for fish.
	*/
	FishBody* fish_body = new   FishBody(system, "FishBody");
	FishMaterial   *fish_body_material = new FishMaterial();
	ElasticSolidParticles  fish_body_particles(fish_body, fish_body_material);
	/**
	* @brief   Particle and body creation of gate observer.
	*/
	Observer* fish_observer = new Observer(system, "Observer");
	BaseParticles           observer_particles(fish_observer);
	/** topology */
	InnerBodyRelation* water_block_inner = new InnerBodyRelation(water_block);
	InnerBodyRelation* fish_body_inner = new InnerBodyRelation(fish_body);
	ComplexBodyRelation* water_block_complex = new ComplexBodyRelation(water_block_inner, { wall_boundary, fish_body });
	ContactBodyRelation* fish_body_contact = new ContactBodyRelation(fish_body, { water_block });
	ContactBodyRelation* fish_observer_contact = new ContactBodyRelation(fish_observer, { fish_body });
	/** Output. */
	In_Output in_output(system);
	WriteBodyStatesToVtu        write_real_body_states(in_output, system.real_bodies_);
	WriteTotalForceOnSolid      write_total_force_on_fish(in_output, fish_body);
	WriteAnObservedQuantity<indexVector, Vecd> write_fish_displacement("Position", in_output, fish_observer_contact);

	/** check whether run particle relaxation for body fitted particle distribution. */
	if (system.run_particle_relaxation_) {
		/**
		 * @brief 	Methods used for particle relaxation.
		 */
		 /** Random reset the insert body particle position. */
		RandomizePartilePosition  random_fish_body_particles(fish_body);
		/** Write the body state to Vtu file. */
		WriteBodyStatesToVtu 		write_fish_body(in_output, { fish_body });
		/** Write the particle reload files. */
		WriteReloadParticle 		write_particle_reload_files(in_output, { fish_body });

		/** A  Physics relaxation step. */
		relax_dynamics::RelaxationStepInner relaxation_step_inner(fish_body_inner);
		/**
		  * @brief 	Particle relaxation starts here.
		  */
		random_fish_body_particles.parallel_exec(0.25);
		relaxation_step_inner.surface_bounding_.parallel_exec();
		write_fish_body.WriteToFile(0.0);

		/** relax particles of the insert body. */
		int ite_p = 0;
		while (ite_p < 1000)
		{
			relaxation_step_inner.parallel_exec();
			ite_p += 1;
			if (ite_p % 200 == 0)
			{
				cout << fixed << setprecision(9) << "Relaxation steps for the inserted body N = " << ite_p << "\n";
				write_fish_body.WriteToFile(Real(ite_p) * 1.0e-4);
			}
		}
		std::cout << "The physics relaxation process of inserted body finish !" << std::endl;

		/** Output results. */
		write_particle_reload_files.WriteToFile(0.0);
		return 0;
	}

	/**
	* This section define all numerical methods will be used in this case.
	*/
	/**
	* @brief   Methods used for updating data structure.
	*/
	/** Periodic BCs in x direction. */
	PeriodicConditionInAxisDirectionUsingCellLinkedList 	periodic_condition(water_block, 0);
	/** Corrected strong configuration.*/
	solid_dynamics::CorrectConfiguration
		fish_body_corrected_configuration_in_strong_form(fish_body_inner);
	/**
	* Common particle dynamics.
	*/
	InitializeATimeStep
		initialize_a_fluid_step(water_block);

	/** Evaluation of density by summation approach. */
	fluid_dynamics::DensitySummationComplex	update_density_by_summation(water_block_complex);
	/** Time step size without considering sound wave speed. */
	fluid_dynamics::AdvectionTimeStepSize	get_fluid_advection_time_step_size(water_block, U_f);
	/** Time step size with considering sound wave speed. */
	fluid_dynamics::AcousticTimeStepSize		get_fluid_time_step_size(water_block);
	/** Pressure relaxation using verlet time stepping. */
	fluid_dynamics::PressureRelaxationWithWall	pressure_relaxation(water_block_complex);
		fluid_dynamics::PressureRelaxationRiemannWithWall	density_relaxation(water_block_complex);
	/** Computing viscous acceleration. */
	fluid_dynamics::ViscousAccelerationWithWall	viscous_acceleration(water_block_complex);
	/** Impose transport velocity formulation. */
	fluid_dynamics::TransportVelocityCorrectionComplex	transport_velocity_correction(water_block_complex);
	/** Computing vorticity in the flow. */
	fluid_dynamics::VorticityInner
		compute_vorticity(water_block_inner);
	/** Inflow boundary condition. */
	ParabolicInflow parabolic_inflow(water_block, new InflowBuffer(water_block, "Buffer"));

	/**
	* Fluid structure interaction model.
	*/
	solid_dynamics::FluidPressureForceOnSolid
		fluid_pressure_force_on_fish_body(fish_body_contact);
	solid_dynamics::FluidViscousForceOnSolid
		fluid_viscous_force_on_fish_body(fish_body_contact);
	/**
	* Solid dynamics.
	*/
	/** Time step size calculation. */
	solid_dynamics::AcousticTimeStepSize fish_body_computing_time_step_size(fish_body);
	/** Process of stress relaxation. */
	solid_dynamics::StressRelaxationFirstHalf
		fish_body_stress_relaxation_first_half(fish_body_inner);
	solid_dynamics::StressRelaxationSecondHalf
		fish_body_stress_relaxation_second_half(fish_body_inner);
	/** Update normal direction on fish body.*/
	solid_dynamics::UpdateElasticNormalDirection
		fish_body_update_normal(fish_body);
	/** Compute the average velocity on fish body. */
	solid_dynamics::AverageVelocityAndAcceleration	fish_body_average_velocity(fish_body);
	/**
	* The multi body system from simbody.
	*/
	SimTK::MultibodySystem          MBsystem;
	/** The bodies or matter of the MBsystem. */
	SimTK::SimbodyMatterSubsystem   matter(MBsystem);
	/** The forces of the MBsystem.*/
	SimTK::GeneralForceSubsystem    forces(MBsystem);
	SimTK::CableTrackerSubsystem    cables(MBsystem);
	/** Mass proeprties of the fixed spot. */
	SimTK::Body::Rigid      fixed_spot_info(SimTK::MassProperties(1.0, Vec3d(0), SimTK::UnitInertia(1)));
	FishHead* fish_head = new FishHead(fish_body, "FishHead", rho0_s);
	/** Mass properties of the consrained spot. */
	SimTK::Body::Rigid      tethered_spot_info(*fish_head->body_part_mass_properties_);
	/** Mobility of the fixed spot. */
	SimTK::MobilizedBody::Weld      fixed_spot(matter.Ground(), SimTK::Transform(tethering_point),
		fixed_spot_info, SimTK::Transform(Vec3d(0)));
	/** Mobility of the tethered spot.
	  * Set the mass center as the origin location of the planar mobilizer
	  */
	Vec3d displacement0 = fish_head->initial_mass_center_ - tethering_point;
	SimTK::MobilizedBody::Planar    tethered_spot(fixed_spot,
		SimTK::Transform(displacement0), tethered_spot_info, SimTK::Transform(Vec3d(0)));
	/** The tethering line give cable force.
	  * the start point of the cable path is at the origin location of the first mobilizer body,
	  * the end point is the tip of the fish head which has a distance to the origin
	  * location of the second mobilizer body origin location, here, the mass center
	  * of the fish head.
	  */
	Vec3d displacement_cable_end = Vec3d(cx, cy, 0.0) - fish_head->initial_mass_center_;
	SimTK::CablePath    tethering_line(cables, fixed_spot,
		Vec3d(0), tethered_spot, displacement_cable_end);
	SimTK::CableSpring  tethering_spring(forces, tethering_line, 100.0, 3.0, 10.0);

	//discreted forces acting on the bodies
	SimTK::Force::DiscreteForces force_on_bodies(forces, matter);
	fixed_spot_info.addDecoration(SimTK::Transform(), SimTK::DecorativeSphere(0.02));
	tethered_spot_info.addDecoration(SimTK::Transform(), SimTK::DecorativeSphere(0.4));
	/** Visualizer from simbody. */
	SimTK::Visualizer viz(MBsystem);
	MBsystem.addEventReporter(new SimTK::Visualizer::Reporter(viz, 0.01));
	/** Initialize the system and state. */
	SimTK::State state = MBsystem.realizeTopology();
	viz.report(state);
	cout << "Hit ENTER to run a short simulation ...";
	getchar();
	/** Time steping method for multibody system.*/
	SimTK::RungeKuttaMersonIntegrator integ(MBsystem);
	integ.setAccuracy(1e-3);
	integ.setAllowInterpolation(false);
	integ.initialize(state);
	/**
	* Coupling between SimBody and SPH.
	*/
	solid_dynamics::TotalForceOnSolidBodyPartForSimBody
		force_on_tethered_spot(fish_body, fish_head,
			MBsystem, tethered_spot, force_on_bodies, integ);
	solid_dynamics::ConstrainSolidBodyPartBySimBody
		constraint_tethered_spot(fish_body,
			fish_head, MBsystem, tethered_spot, force_on_bodies, integ);

	/**
	* Time steeping starts here.
	*/
	GlobalStaticVariables::physical_time_ = 0.0;
	/** Using relaxed particle distribution if needed. */
	if (system.reload_particles_) {
		ReadReloadParticle		reload_insert_body_particles(in_output, { fish_body }, { "FishBody" });
		reload_insert_body_particles.ReadFromFile();
	}
	/**
	* Initial periodic boundary condition which copies the particle identifies
	* as extra cell linked list form periodic regions to the corresponding boundaries
	* for building up of extra configuration.
	*/
	system.initializeSystemCellLinkedLists();
	periodic_condition.update_cell_linked_list_.parallel_exec();
	system.initializeSystemConfigurations();
	/** Prepare quantities, e.g. wall normal, fish body norm,
	* fluid initial number density and configuration of fish particles, will be used once only.
	*/
	wall_particles.initializeNormalDirectionFromGeometry();
	fish_body_particles.initializeNormalDirectionFromGeometry();
	fish_body_corrected_configuration_in_strong_form.parallel_exec();
	/** Output for initial condition. */
	write_real_body_states.WriteToFile(GlobalStaticVariables::physical_time_);
	write_fish_displacement.WriteToFile(GlobalStaticVariables::physical_time_);
	/**
	* Time parameters
	*/
	int number_of_iterations = 0;
	int screen_output_interval = 100;
	Real End_Time = 200.0;
	Real D_Time = End_Time / 200.0;
	Real Dt = 0.0;      /**< Default advection time step sizes. */
	Real dt = 0.0;      /**< Default acoustic time step sizes. */
	Real dt_s = 0.0;	/**< Default acoustic time step sizes for solid. */
	tick_count t1 = tick_count::now();
	tick_count::interval_t interval;

	/**
	* Main loop starts here.
	*/
	while (GlobalStaticVariables::physical_time_ < End_Time)
	{
		Real integration_time = 0.0;
		while (integration_time < D_Time)
		{
			initialize_a_fluid_step.parallel_exec();
			Dt = get_fluid_advection_time_step_size.parallel_exec();
			update_density_by_summation.parallel_exec();
			viscous_acceleration.parallel_exec();
			transport_velocity_correction.parallel_exec(Dt);
			/** Viscous force exerting on fish body. */
			fluid_viscous_force_on_fish_body.parallel_exec();
			/** Update normal direction on fish body. */
			fish_body_update_normal.parallel_exec();
			Real relaxation_time = 0.0;
			while (relaxation_time < Dt) 
			{
				//note that dt needs to sufficiently large to avoid divide zero
				//when computing solid avergare velocity for FSI
				dt = SMIN(get_fluid_time_step_size.parallel_exec(), Dt);
				/** Fluid dynamics process, first half. */
				pressure_relaxation.parallel_exec(dt);
				/** Fluid pressure force exerting on fish. */
				fluid_pressure_force_on_fish_body.parallel_exec();
				/** Fluid dynamics process, second half. */
				density_relaxation.parallel_exec(dt);
				/** Relax fish body by solid dynamics. */
				Real dt_s_sum = 0.0;
				fish_body_average_velocity.initialize_displacement_.parallel_exec();
				while (dt_s_sum < dt)
				{
					dt_s = SMIN(fish_body_computing_time_step_size.parallel_exec(), dt - dt_s_sum);
					fish_body_stress_relaxation_first_half.parallel_exec(dt_s);
					SimTK::State& state_for_update = integ.updAdvancedState();
					force_on_bodies.clearAllBodyForces(state_for_update);
					force_on_bodies.setOneBodyForce(state_for_update, tethered_spot,
						force_on_tethered_spot.parallel_exec());
					integ.stepBy(dt_s);
					constraint_tethered_spot.parallel_exec();
					fish_body_stress_relaxation_second_half.parallel_exec(dt_s);
					dt_s_sum += dt_s;
				}
				//note that dt needs to sufficiently large to avoid divide zero
				fish_body_average_velocity.update_averages_.parallel_exec(dt);
				write_total_force_on_fish.WriteToFile(GlobalStaticVariables::physical_time_);

				relaxation_time += dt;
				integration_time += dt;
				GlobalStaticVariables::physical_time_ += dt;
				parabolic_inflow.parallel_exec();

			}
			if (number_of_iterations % screen_output_interval == 0)
			{
				cout << fixed << setprecision(9) << "N=" << number_of_iterations << "	Time = "
					<< GlobalStaticVariables::physical_time_
					<< "	Dt = " << Dt << "	dt = " << dt << "	dt_s = " << dt_s << "\n";
			}
			number_of_iterations++;

			//visualize the motion of rigid body
			viz.report(integ.getState());
			/** Water block configuration and periodic condition. */
			periodic_condition.bounding_.parallel_exec();
			water_block->updateCellLinkedList();
			fish_body->updateCellLinkedList();
			periodic_condition.update_cell_linked_list_.parallel_exec();
			water_block_complex->updateConfiguration();
			/** Fish body contact configuration. */
			fish_body_contact->updateConfiguration();
			write_fish_displacement.WriteToFile(GlobalStaticVariables::physical_time_);
		}
		tick_count t2 = tick_count::now();
		compute_vorticity.parallel_exec();
		write_real_body_states.WriteToFile(GlobalStaticVariables::physical_time_ * 0.001);
		tick_count t3 = tick_count::now();
		interval += t3 - t2;
	}
	tick_count t4 = tick_count::now();

	tick_count::interval_t tt;
	tt = t4 - t1 - interval;
	cout << "Total wall time for computation: " << tt.seconds() << " seconds." << endl;

	return 0;
}
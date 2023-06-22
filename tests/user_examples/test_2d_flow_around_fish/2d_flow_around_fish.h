
#include "sphinxsys.h"
#include "2d_fish_and_bones.h"
#include "composite_material.h"
#define PI 3.1415926
using namespace SPH;
//----------------------------------------------------------------------
//	Basic geometry parameters and numerical setup.
//----------------------------------------------------------------------
Real DL = 0.8;								     /**< Channel length. */
Real DH = 0.4;								    /**< Channel height. */
Real particle_spacing_ref = 0.0025;			   /**< Initial reference particle spacing. */
Real DL_sponge = particle_spacing_ref * 20.0; /**< Sponge region to impose inflow condition. */
Real BW = particle_spacing_ref * 4.0;		 /**< Boundary width, determined by specific layer of boundary particles. */
/** Domain bounds of the system. */
BoundingBox system_domain_bounds(Vec2d(-DL_sponge - BW, -BW), Vec2d(DL + BW, DH + BW));

Vec2d buffer_halfsize = Vec2d(0.5 * DL_sponge, 0.5 * DH);
Vec2d buffer_translation = Vec2d(-DL_sponge, 0.0) + buffer_halfsize;
//----------------------------------------------------------------------
//	Material properties of the fluid.
//----------------------------------------------------------------------
Real rho0_f = 1000.0;										 /**< Density. */
Real U_f = 1.0;												/**< freestream velocity. */
Real c_f = 10.0 * U_f;									   /**< Speed of sound. */
Real Re = 30000.0;										  /**< Reynolds number. */
Real mu_f = rho0_f * U_f * 0.3 / Re;                     /**< Dynamics viscosity. */
//----------------------------------------------------------------------
//	Global parameters on the solid properties
//----------------------------------------------------------------------
//----------------------------------------------------------------------
Real cx = 0.3 * DL;			  /**< Center of fish in x direction. */
Real cy = DH / 2;             /**< Center of fish in y direction. */
Real fish_length = 0.2;       /**< Length of fish. */
Real fish_thickness = 0.03;   /**< The maximum fish thickness. */
Real muscel_thickness = 0.02; /**< The maximum fish thickness. */
Real head_length = 0.03;      /**< Length of fish bone. */
Real bone_thickness = 0.003;  /**< Length of fish bone. */
Real fish_shape_resolution = particle_spacing_ref * 0.5;

Real rho0_s = 1050.0;
Real Youngs_modulus1 = 0.8e6;
Real Youngs_modulus2 = 0.5e6;
Real Youngs_modulus3 = 1.1e6;
Real poisson = 0.49;

Real a1 = 1.22 * fish_thickness / fish_length;
Real a2 = 3.19 * fish_thickness / fish_length / fish_length;
Real a3 = -15.73 * fish_thickness / pow(fish_length, 3);
Real a4 = 21.87 * fish_thickness / pow(fish_length, 4);
Real a5 = -10.55 * fish_thickness / pow(fish_length, 5);

Real b1 = 1.22 * muscel_thickness / fish_length;
Real b2 = 3.19 * muscel_thickness / fish_length / fish_length;
Real b3 = -15.73 * muscel_thickness / pow(fish_length, 3);
Real b4 = 21.87 * muscel_thickness / pow(fish_length, 4);
Real b5 = -10.55 * muscel_thickness / pow(fish_length, 5);
//----------------------------------------------------------------------
//	SPH bodies with cases-dependent geometries (ComplexShape).
//----------------------------------------------------------------------
/** create a water block shape */
std::vector<Vecd> createWaterBlockShape()
{
	//geometry
	std::vector<Vecd> water_block_shape;
	water_block_shape.push_back(Vecd(-DL_sponge, 0.0));
	water_block_shape.push_back(Vecd(-DL_sponge, DH));
	water_block_shape.push_back(Vecd(DL, DH));
	water_block_shape.push_back(Vecd(DL, 0.0));
	water_block_shape.push_back(Vecd(-DL_sponge, 0.0));

	return water_block_shape;
}

/** create outer wall shape */
std::vector<Vecd> createOuterWallShape()
{
	std::vector<Vecd> outer_wall_shape;
	outer_wall_shape.push_back(Vecd(-DL_sponge - BW, -BW));
	outer_wall_shape.push_back(Vecd(-DL_sponge - BW, DH + BW));
	outer_wall_shape.push_back(Vecd(DL + BW, DH + BW));
	outer_wall_shape.push_back(Vecd(DL + BW, -BW));
	outer_wall_shape.push_back(Vecd(-DL_sponge - BW, -BW));

	return outer_wall_shape;
}
/**
* @brief create inner wall shape
*/
std::vector<Vecd> createInnerWallShape()
{
	std::vector<Vecd> inner_wall_shape;
	inner_wall_shape.push_back(Vecd(-DL_sponge - 2.0 * BW, 0.0));
	inner_wall_shape.push_back(Vecd(-DL_sponge - 2.0 * BW, DH));
	inner_wall_shape.push_back(Vecd(DL + 2.0 * BW, DH));
	inner_wall_shape.push_back(Vecd(DL + 2.0 * BW, 0.0));
	inner_wall_shape.push_back(Vecd(-DL_sponge - 2.0 * BW, 0.0));

	return inner_wall_shape;
}
/**
* Fish body with tethering constraint.
*/
class FishBody : public MultiPolygonShape
{

public:
	explicit FishBody(const std::string &shape_name) : MultiPolygonShape(shape_name)
	{
		std::vector<Vecd> fish_shape = CreatFishShape(cx, cy, fish_length, fish_shape_resolution);
		multi_polygon_.addAPolygon(fish_shape, ShapeBooleanOps::add);
	}
};
//----------------------------------------------------------------------
//	Define case dependent bodies material, constraint and boundary conditions.
//----------------------------------------------------------------------
/** Fluid body definition */
class WaterBlock : public ComplexShape
{
public:
	explicit WaterBlock(const std::string& shape_name) : ComplexShape(shape_name)
	{
		/** Geomtry definition. */
		MultiPolygon outer_boundary(createWaterBlockShape());
		add<MultiPolygonShape>(outer_boundary, "OuterBoundary");
		MultiPolygon fish(CreatFishShape(cx, cy, fish_length, fish_shape_resolution));
		subtract<MultiPolygonShape>(fish);
	}
};

/* Definition of the solid body. */
class WallBoundary : public MultiPolygonShape
{
public:
	explicit WallBoundary(const std::string &shape_name) : MultiPolygonShape(shape_name)
	{
		std::vector<Vecd> outer_shape = createOuterWallShape();
		std::vector<Vecd> inner_shape = createInnerWallShape();
		multi_polygon_.addAPolygon(outer_shape, ShapeBooleanOps::add);
		multi_polygon_.addAPolygon(inner_shape, ShapeBooleanOps::sub);
	}
};

/** create a inflow buffer shape. */
MultiPolygon createInflowBufferShape()
{
	std::vector<Vecd> inflow_buffer_shape;
	inflow_buffer_shape.push_back(Vecd(-DL_sponge, 0.0));
	inflow_buffer_shape.push_back(Vecd(-DL_sponge, DH));
	inflow_buffer_shape.push_back(Vecd(0.0, DH));
	inflow_buffer_shape.push_back(Vecd(0.0, 0.0));
	inflow_buffer_shape.push_back(Vecd(-DL_sponge, 0.0));

	MultiPolygon multi_polygon;
	multi_polygon.addAPolygon(inflow_buffer_shape, ShapeBooleanOps::add);
	return multi_polygon;
}
/** Case dependent inflow boundary condition. */
struct InflowVelocity
{
	Real u_ref_, t_ref_;
	AlignedBoxShape& aligned_box_;
	Vecd halfsize_;

	template <class BoundaryConditionType>
	InflowVelocity(BoundaryConditionType& boundary_condition)
		: u_ref_(0.00), t_ref_(2.0),
		aligned_box_(boundary_condition.getAlignedBox()),
		halfsize_(aligned_box_.HalfSize()) {}

	Vecd operator()(Vecd& position, Vecd& velocity)
	{
		Vecd target_velocity = Vecd::Zero();
		Real run_time = GlobalStaticVariables::physical_time_;
		Real u_ave = run_time < t_ref_ ? 0.5 * u_ref_ * (1.0 - cos(Pi * run_time / t_ref_)) : u_ref_;	
	    target_velocity[0] = u_ave;		
		return target_velocity;
	}
};

//Material ID
class SolidBodyMaterial : public CompositeMaterial
{
public:
	SolidBodyMaterial() : CompositeMaterial(rho0_s)
	{
		add<ActiveModelSolid>(rho0_s, Youngs_modulus1, poisson);
		add<SaintVenantKirchhoffSolid>(rho0_s, Youngs_modulus2, poisson);
		add<SaintVenantKirchhoffSolid>(rho0_s, Youngs_modulus3, poisson);
	};
};

//	Setup material ID
class MaterialId
	: public solid_dynamics::ElasticDynamicsInitialCondition
{
public:
	explicit  MaterialId(SolidBody& solid_body)
		: solid_dynamics::ElasticDynamicsInitialCondition(solid_body),
		solid_particles_(dynamic_cast<SolidParticles*>(&solid_body.getBaseParticles())),
		materail_id_(*solid_particles_->getVariableByName<int>("MaterailId")),
		pos0_(solid_particles_->pos0_)
	{
		solid_particles_->registerVariable(active_strain_, "ActiveStrain");
	};
	virtual void update(size_t index_i, Real dt = 0.0)
	{
		Real x = pos0_[index_i][0]-cx;
		Real y = pos0_[index_i][1];
		Real y1(0);

		y1 = a1 * pow(x, 0 + 1) + a2 * pow(x, 1 + 1) + a3 * pow(x, 2 + 1) + a4 * pow(x, 3 + 1) + a5 * pow(x, 4 + 1);

		Real Am = 0.12;
		Real frequency = 4.0;
		Real w = 2 * PI * frequency;
		Real lamda = 3.0 * fish_length;
		Real wave_number = 2 * PI / lamda;
		Real hx = -(pow(x, 2)- pow(fish_length, 2)) / pow(fish_length, 2);
		Real ta = 0.2;
		Real st = 1 - exp(-GlobalStaticVariables::physical_time_ / ta);

		active_strain_[index_i] = Matd::Zero();

		if (x <=(fish_length - head_length)  && y > (y1-0.004 + cy) && y > (cy + bone_thickness / 2))
		{
			materail_id_[index_i] = 0;
			active_strain_[index_i](0, 0) = -Am * hx * st * pow(sin(w * GlobalStaticVariables::physical_time_/2 + wave_number * x/2), 2);
		}
		else if (x <= (fish_length - head_length)  && y < (-y1 + 0.004 + cy) && y <(cy - bone_thickness / 2))
		{
			materail_id_[index_i] = 0;
			active_strain_[index_i](0, 0) = -Am * hx * st * pow(sin(w * GlobalStaticVariables::physical_time_/2 + wave_number * x/2 + PI / 2), 2);
		}
		 else if ((x > (fish_length - head_length)) || ((y < (cy + bone_thickness / 2)) && (y > (cy - bone_thickness / 2))))
		{
			materail_id_[index_i] = 2;
		}
		else
		{
			materail_id_[index_i] = 1;
		}
	};
protected:
	SolidParticles* solid_particles_;
	StdLargeVec<int>& materail_id_;
	StdLargeVec<Vecd>& pos0_;
	StdLargeVec<Matd> active_strain_;
};

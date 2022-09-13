
/**
 * @file 	io_plt.cpp
 * @author	Luhui Han, Chi ZHang and Xiangyu Hu
 */

#include "io_plt.h"

namespace SPH
{
	//=============================================================================================//
	void PltEngine::
		writeAQuantityHeader(std::ofstream &out_file, const Real &quantity, const std::string &quantity_name)
	{
		out_file << "\"" << quantity_name << "\""
				 << "   ";
	}
	//=============================================================================================//
	void PltEngine::
		writeAQuantityHeader(std::ofstream &out_file, const Vecd &quantity, const std::string &quantity_name)
	{
		for (int i = 0; i != Dimensions; ++i)
			out_file << "\"" << quantity_name << "[" << i << "]\""
					 << "   ";
	}
	//=============================================================================================//
	void PltEngine::writeAQuantity(std::ofstream &out_file, const Real &quantity)
	{
		out_file << std::fixed << std::setprecision(9) << quantity << "   ";
	}
	//=============================================================================================//
	void PltEngine::writeAQuantity(std::ofstream &out_file, const Vecd &quantity)
	{
		for (int i = 0; i < Dimensions; ++i)
			out_file << std::fixed << std::setprecision(9) << quantity[i] << "   ";
	}
	//=============================================================================================//
	void BodyStatesRecordingToPlt::writeWithFileName(const std::string &sequence)
	{
		for (SPHBody *body : bodies_)
		{
			if (body->checkNewlyUpdated())
			{
				std::string filefullpath = io_environment_.output_folder_ + "/SPHBody_" + body->getName() + "_" + sequence + ".plt";
				if (fs::exists(filefullpath))
				{
					fs::remove(filefullpath);
				}
				std::ofstream out_file(filefullpath.c_str(), std::ios::trunc);

				// begin of the plt file writing

				body->writeParticlesToPltFile(out_file);

				out_file.close();
			}
			body->setNotNewlyUpdated();
		}
	}
	//=============================================================================================//
	MeshRecordingToPlt ::MeshRecordingToPlt(IOEnvironment &io_environment, SPHBody &body, BaseMeshField *mesh_field)
		: BodyStatesRecording(io_environment, body), mesh_field_(mesh_field)
	{
		filefullpath_ = io_environment_.output_folder_ + "/" + body.getName() + "_" + mesh_field_->Name() + ".dat";
	}
	//=============================================================================================//
	void MeshRecordingToPlt::writeWithFileName(const std::string &sequence)
	{
		std::ofstream out_file(filefullpath_.c_str(), std::ios::app);
		mesh_field_->writeMeshFieldToPlt(out_file);
		out_file.close();
	}
	//=================================================================================================//
}

#include "../voxelizer.h"
#include "../writer.h"
#include "../processor.h"

#if defined(WITH_IFC) && defined(IFCOPENSHELL_05)
#include <ifcparse/IfcFile.h>
#include <ifcgeom/IfcGeomIterator.h>
using namespace Ifc2x3;
#endif

#include <BRep_Builder.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#include <gtest/gtest.h>


#ifdef WIN32
#define DIRSEP "\\"
#else
#define DIRSEP "/"
#endif

#if defined(WITH_IFC) && defined(IFCOPENSHELL_05)
TEST(Voxelization, IfcCovering) {
	const std::string input_filename = ".." DIRSEP "tests" DIRSEP "fixtures" DIRSEP "covering.ifc";

	IfcParse::IfcFile ifc_file;

	const double d = 0.05;

	ASSERT_TRUE(ifc_file.Init(input_filename));

	IfcGeom::IteratorSettings settings_surface;
	settings_surface.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, true);

	IfcGeom::Kernel kernel;
	kernel.initializeUnits(*ifc_file.entitiesByType<IfcSchema::IfcUnitAssignment>()->begin());

	IfcBuildingElement* buildingelem = ifc_file.entityByGuid("00fSj8UXfAn8lUGu3HP6DA")->as<IfcBuildingElement>();
	IfcShapeRepresentation* representation = nullptr;
	auto reps = ifc_file.traverse(buildingelem)->as<IfcShapeRepresentation>();
	for (auto& rep : *reps) {
		if (rep->RepresentationIdentifier() == "Body") {
			representation = rep;
		}
	}

	ASSERT_NE(representation, nullptr);

	auto elem = kernel.create_brep_for_representation_and_product<double>(settings_surface, representation, buildingelem);

	const gp_Trsf& product_trsf = elem->transformation().data();
	Bnd_Box global_bounds;
	TopoDS_Compound compound;
	BRep_Builder builder;
	builder.MakeCompound(compound);

	for (IfcGeom::IfcRepresentationShapeItems::const_iterator it = elem->geometry().begin(); it != elem->geometry().end(); ++it) {
		const TopoDS_Shape& s = it->Shape();
		gp_GTrsf trsf = it->Placement();
		const TopoDS_Shape moved_shape_ = IfcGeom::Kernel::apply_transformation(s, trsf);
		const TopoDS_Shape moved_shape = IfcGeom::Kernel::apply_transformation(moved_shape_, product_trsf);
		BRepBndLib::Add(moved_shape, global_bounds);
		builder.Add(compound, moved_shape);
	}

	BRepMesh_IncrementalMesh(compound, 0.001);

	geometry_collection_t geoms{ { buildingelem->id(), compound } };

	double x1, y1, z1, x2, y2, z2;
	global_bounds.Get(x1, y1, z1, x2, y2, z2);
	int nx = (int)ceil((x2 - x1) / d) + 10;
	int ny = (int)ceil((y2 - y1) / d) + 10;
	int nz = (int)ceil((z2 - z1) / d) + 10;

	x1 -= d * 5;
	y1 -= d * 5;
	z1 -= d * 5;

	{
		progress_writer progress_1("test_surface");
		processor pr(x1, y1, z1, d, nx, ny, nz, 64, progress_1);
		pr.process(geoms.begin(), geoms.end(), SURFACE(), output(MERGED(), "test_covering_surface.vox"));
	}
}
#else
TEST(Voxelization, DISABLED_IfcCovering) {}
#endif
